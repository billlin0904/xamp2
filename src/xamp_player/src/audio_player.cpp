#include <base/str_utilts.h>
#include <base/platform.h>
#include <base/logger.h>
#include <base/stl.h>
#include <base/threadpool.h>
#include <base/dsdsampleformat.h>
#include <base/dataconverter.h>
#include <base/dsdsampleformat.h>
#include <base/buffer.h>
#include <base/timer.h>

#include <output_device/audiodevicemanager.h>
#include <output_device/asiodevicetype.h>
#include <output_device/dsddevice.h>

#include <stream/bassfilestream.h>
#include <stream/audioprocessor.h>

#include <player/soxresampler.h>
#include <player/samplerateconverter.h>
#include <player/passthroughsamplerateconverter.h>
#include <player/chromaprint.h>
#include <player/audio_util.h>
#include <player/audio_player.h>

namespace xamp::player {

inline constexpr int32_t kBufferStreamCount = 5;
inline constexpr int32_t kTotalBufferStreamCount = 10;

inline constexpr uint32_t kPreallocateBufferSize = 4 * 1024 * 1024;
inline constexpr uint32_t kMaxPreallocateBufferSize = 16 * 1024 * 1024;
	
inline constexpr uint32_t kMaxWriteRatio = 80;
inline constexpr uint32_t kMaxReadRatio = 2;
inline constexpr uint32_t kMsgQueueSize = 30;
	
inline constexpr uint32_t kDefaultReadSample = 8192 * 4;

inline constexpr std::chrono::milliseconds kUpdateSampleInterval(100);
inline constexpr std::chrono::milliseconds kReadSampleWaitTime(30);
inline constexpr std::chrono::seconds kWaitForStreamStopTime(10);
inline constexpr std::chrono::milliseconds kPauseWaitTimeout(100);

#ifdef _DEBUG
static void LogTime(const std::string & msg, const std::chrono::microseconds &time) {
    auto c = time.count();
    XAMP_LOG_DEBUG(msg + ": {}.{:03}'{:03}sec",
        (c % 1'000'000'000) / 1'000'000, (c % 1'000'000) / 1'000, c % 1'000);
}
#endif

AudioPlayer::AudioPlayer()
    : AudioPlayer(std::weak_ptr<PlaybackStateAdapter>()) {
}

AudioPlayer::AudioPlayer(const std::weak_ptr<PlaybackStateAdapter> &adapter)
    : is_muted_(false)
    , enable_sample_converter_(false)
	, enable_processor_(true)
	, is_dsd_file_(false)
    , dsd_mode_(DsdModes::DSD_MODE_PCM)
    , state_(PlayerState::PLAYER_STATE_STOPPED)
    , sample_size_(0)
    , target_sample_rate_(0)
    , volume_(0)
    , fifo_size_(0)
    , num_read_sample_(0)
    , is_playing_(false)
    , is_paused_(false)
    , sample_end_time_(0)
    , stream_duration_(0)
    , state_adapter_(adapter)
    , fifo_(GetPageAlignSize(kPreallocateBufferSize))
    , seek_queue_(kMsgQueueSize)
	, processor_queue_(kMsgQueueSize) {
    logger_ = Logger::GetInstance().GetLogger(kAudioPlayerLoggerName);
}

AudioPlayer::~AudioPlayer() = default;

void AudioPlayer::Destroy() {
    timer_.Stop();
    try {
        CloseDevice(true);
    }
    catch (...) {
    }
    Stop(false, true);
    stream_.reset();
    converter_.reset();
    read_buffer_.reset();
#ifdef ENABLE_ASIO
    AudioDeviceManager::RemoveASIODriver();
#endif
    FreeBassLib();
}

void AudioPlayer::UpdateSlice(int32_t sample_size, double stream_time) noexcept {
    std::atomic_exchange_explicit(&slice_,
        AudioSlice{ sample_size, stream_time },
        std::memory_order_relaxed);
}

void AudioPlayer::LoadDecoder() {
    AudioDeviceManager::PreventSleep(true);
    XAMP_LOG_DEBUG("AudioDeviceManager init success.");

    LoadBassLib();
    XAMP_LOG_DEBUG("Load BASS dll success.");

    LoadSoxrLib();
    XAMP_LOG_DEBUG("Load Soxr dll success.");
	
    try {
        Chromaprint::LoadChromaprintLib();
        XAMP_LOG_DEBUG("Load Chromaprint dll success.");
    }
    catch (...) {
    	// Ignore exception.
    }    
}

void AudioPlayer::Open(Path const& file_path) {
    auto device = device_manager_.CreateDefaultDeviceType();
    device->ScanNewDevice();
    if (auto device_info = device->GetDefaultDeviceInfo()) {
        Open(file_path, *device_info);
    } else {
        throw DeviceNotFoundException();
    }
}

void AudioPlayer::Open(Path const& file_path, const DeviceInfo& device_info, AlignPtr<SampleRateConverter> converter) {
    Startup();
    CloseDevice(true);
    enable_sample_converter_ = converter != nullptr;
    converter_  = std::move(converter);
    target_sample_rate_ = 0;
    OpenStream(file_path, device_info);
    device_info_ = device_info;
}

void AudioPlayer::SetTargetSampleRate(uint32_t target_sample_rate) {
    target_sample_rate_ = target_sample_rate;
}

void AudioPlayer::SetProcessor(AlignPtr<AudioProcessor>&& processor) {
    processor_queue_.TryEnqueue(std::move(processor));
}

void AudioPlayer::EnableProcessor(bool enable) {
    enable_processor_ = enable;
    XAMP_LOG_DEBUG("Enable processor {}", enable);
}

bool AudioPlayer::IsEnableProcessor() const {
    return enable_processor_;
}

void AudioPlayer::SetDevice(const DeviceInfo& device_info) {
    device_info_ = device_info;
}

DeviceInfo AudioPlayer::GetDevice() const {
    return device_info_;
}

void AudioPlayer::CreateDevice(Uuid const & device_type_id, std::string const & device_id, bool open_always) {
    if (device_ == nullptr
        || device_id_ != device_id
        || device_type_id_ != device_type_id
        || open_always) {
        if (device_type_id_ != device_type_id) {
            AudioDeviceManager::RemoveASIODriver();
            device_.reset();
        }
    	
        device_type_ = device_manager_.Create(device_type_id);
        device_type_->ScanNewDevice();
        device_ = device_type_->MakeDevice(device_id);
        device_type_id_ = device_type_id;
        device_id_ = device_id;
        XAMP_LOG_D(logger_, "Create device: {}", device_type_->GetDescription());
    }
    device_->SetAudioCallback(this);
}

bool AudioPlayer::IsDSDFile() const {
    return is_dsd_file_;
}

void AudioPlayer::OpenStream(Path const& file_path, DeviceInfo const & device_info) {
    auto [dsd_mode, stream] = audio_util::MakeFileStream(file_path, device_info, enable_sample_converter_);
    stream_ = std::move(stream);
    dsd_mode_ = dsd_mode;
    if (auto* dsd_stream = AsDsdStream(stream_)) {
        dsd_stream->SetDSDMode(dsd_mode_);
        if (dsd_stream->IsDsdFile()) {
            is_dsd_file_ = true;
            dsd_speed_ = dsd_stream->GetDsdSpeed();
        } else {
            is_dsd_file_ = false;
            dsd_speed_ = std::nullopt;
        }        
    }
    stream_duration_ = stream_->GetDuration();
    XAMP_LOG_D(logger_, "Open stream type: {} {} duration:{}.", stream_->GetDescription(), dsd_mode_, stream_duration_.load());
}

void AudioPlayer::SetState(const PlayerState play_state) {
    if (auto adapter = state_adapter_.lock()) {
        adapter->OnStateChanged(play_state);
    }
    state_ = play_state;
    XAMP_LOG_D(logger_, "Set state: {}.", EnumToString(state_));
}

void AudioPlayer::ProcessEvent() {
    if (auto const* stream_time = seek_queue_.Front()) {
        XAMP_LOG_D(logger_, "Receive seek {} message", *stream_time);
        DoSeek(*stream_time);
        seek_queue_.Pop();
    }
}

void AudioPlayer::InitProcessor() {
	while (!processor_queue_.empty()) {
        if (auto* processor = processor_queue_.Front()) {
            auto id = (*processor)->GetTypeId();
            const auto itr = std::find_if(dsp_chain_.begin(),
                dsp_chain_.end(),
                [id](auto const& processor) {
                    return processor->GetTypeId() == id;
                });
            auto& pp = (*processor);
            if (itr != dsp_chain_.end()) {
                (*itr) = std::move(pp);
            }
            else {
                dsp_chain_.push_back(std::move(pp));
            }
            processor_queue_.Pop();
        }
	}    
}
	
void AudioPlayer::Pause() {
    if (!device_) {
        return;
    }

    XAMP_LOG_D(logger_, "Player pasue.");
    if (!is_paused_) {        
        if (device_->IsStreamOpen()) {
            is_paused_ = true;            
            device_->StopStream(false);
            SetState(PlayerState::PLAYER_STATE_PAUSED);            
        }
    }
}

void AudioPlayer::Resume() {
    if (!device_) {
        return;
    }

    XAMP_LOG_D(logger_, "Player resume.");
    if (device_->IsStreamOpen()) {
        SetState(PlayerState::PLAYER_STATE_RESUME);
        is_paused_ = false;
        pause_cond_.notify_all();
        stopped_cond_.notify_all();
#ifdef _DEBUG
        sw_.Reset();
#endif
        device_->StartStream();
        SetState(PlayerState::PLAYER_STATE_RUNNING);
    }
}

void AudioPlayer::Stop(bool signal_to_stop, bool shutdown_device, bool wait_for_stop_stream) {
    if (!device_) {
        return;
    }

    XAMP_LOG_D(logger_, "Player stop.");
    if (device_->IsStreamOpen()) {
        XAMP_LOG_D(logger_, "Close device.");
        CloseDevice(wait_for_stop_stream);
        UpdateSlice();
        if (signal_to_stop) {
            SetState(PlayerState::PLAYER_STATE_STOPPED);                        
        }
    }

    MSleep(500);

    fifo_.Clear();
    if (shutdown_device) {
        XAMP_LOG_D(logger_, "Shutdown device.");
        if (AudioDeviceManager::IsASIODevice(device_type_id_)) {
            device_.reset();
            AudioDeviceManager::RemoveASIODriver();            
        }
        device_id_.clear();
        device_.reset();
    }
    stream_.reset();

#ifdef _DEBUG
    render_thread_id_.clear();
    XAMP_LOG_INFO("AVG max time {} ms", max_process_time_.count() / 1000);
    sw_.Reset();
    max_process_time_ = std::chrono::microseconds(0);
#endif
}

void AudioPlayer::SetVolume(uint32_t volume) {
    volume_ = volume;
    if (!device_ || !device_->IsStreamOpen()) {
        return;
    }
    device_->SetVolume(volume);
}

uint32_t AudioPlayer::GetVolume() const {
    if (!device_ || !device_->IsStreamOpen()) {
        return volume_;
    }
    return device_->GetVolume();
}

bool AudioPlayer::IsHardwareControlVolume() const {
    if (device_ != nullptr && device_->IsStreamOpen()) {
        return device_->IsHardwareControlVolume();
    }
    return true;
}

bool AudioPlayer::IsMute() const {
    if (device_ != nullptr && device_->IsStreamOpen()) {
#ifdef ENABLE_ASIO
        if (device_type_->GetTypeId() == ASIODeviceType::Id) {
            return is_muted_;
        }
#else
        return device_->IsMuted();
#endif
    }
    return is_muted_;
}

void AudioPlayer::SetMute(bool mute) {
    is_muted_ = mute;
    if (!device_ || !device_->IsStreamOpen()) {
        return;
    }
    device_->SetMute(mute);
}

std::optional<uint32_t> AudioPlayer::GetDSDSpeed() const {
    return dsd_speed_;	
}

double AudioPlayer::GetDuration() const {
    if (!stream_) {
        return 0.0;
    }
    return stream_duration_;
}

PlayerState AudioPlayer::GetState() const noexcept {
    return state_;
}

AudioFormat AudioPlayer::GetInputFormat() const noexcept {
    return input_format_;
}

AudioFormat AudioPlayer::GetOutputFormat() const noexcept {
    return output_format_;
}

bool AudioPlayer::IsPlaying() const noexcept {
    return is_playing_;
}

DsdModes AudioPlayer::GetDsdModes() const noexcept {
    return dsd_mode_;
}

void AudioPlayer::CloseDevice(bool wait_for_stop_stream) {
    is_playing_ = false;
    is_paused_ = false;
    pause_cond_.notify_all();
    stopped_cond_.notify_all();
    if (device_ != nullptr) {
        if (device_->IsStreamOpen()) {
            XAMP_LOG_D(logger_, "Stop output device");
            try {
                device_->StopStream(wait_for_stop_stream);
                device_->CloseStream();
            } catch (const Exception &e) {
                XAMP_LOG_D(logger_, "Close device failure. {}", e.what());
            }
        }
    }

    if (stream_task_.valid()) {
        XAMP_LOG_D(logger_, "Try to stop stream thread.");
#ifdef XAMP_OS_WIN
        // MSVC 2019 is wait for std::packaged_task return timeout, while others such clang can't.
        if (stream_task_.wait_for(kWaitForStreamStopTime) == std::future_status::timeout) {
            throw StopStreamTimeoutException();
        }
#else
        stream_task_.get();
#endif
        stream_task_ = std::shared_future<void>();
        XAMP_LOG_D(logger_, "Stream thread was finished.");
    }
    fifo_.Clear();
}

void AudioPlayer::AllocateReadBuffer(uint32_t allocate_size) {
    if (read_buffer_.GetSize() == 0 || read_buffer_.GetSize() != allocate_size) {
        XAMP_LOG_D(logger_, "Allocate read buffer : {}.", String::FormatBytes(allocate_size));
        read_buffer_ = MakeBuffer<int8_t>(allocate_size);
    }
}

void AudioPlayer::AllocateFifo() {
    if (fifo_.GetSize() == 0 || fifo_.GetSize() != fifo_size_) {
        XAMP_LOG_D(logger_, "Allocate internal buffer : {}.", String::FormatBytes(fifo_size_));
        fifo_.Resize(fifo_size_);
    }
}

void AudioPlayer::CreateBuffer() {
    UpdateSlice();

    uint32_t require_read_sample = 0;

    if (dsd_mode_ != DsdModes::DSD_MODE_NATIVE) {
	    require_read_sample = device_->GetBufferSize() * kMaxReadRatio;
    } else {
        require_read_sample = output_format_.GetSampleRate() / 8;
    }

    require_read_sample = std::max(require_read_sample, kDefaultReadSample);
    uint32_t allocate_read_size = 0;
    if (dsd_mode_ == DsdModes::DSD_MODE_NATIVE) {
        allocate_read_size = kMaxPreallocateBufferSize;
    }
    else {
        allocate_read_size = (std::min)(kMaxPreallocateBufferSize,
            require_read_sample * stream_->GetSampleSize() * kBufferStreamCount);
        allocate_read_size = AlignUp(allocate_read_size);
    }
    fifo_size_ = allocate_read_size * kTotalBufferStreamCount;
    num_read_sample_ = require_read_sample;
    AllocateReadBuffer(allocate_read_size);
    AllocateFifo();

    if (!enable_sample_converter_
        || dsd_mode_ == DsdModes::DSD_MODE_NATIVE
        || dsd_mode_ == DsdModes::DSD_MODE_DOP) {
        converter_ = MakeAlign<SampleRateConverter, PassThroughSampleRateConverter>(dsd_mode_, stream_->GetSampleSize());
    } else {
        if (target_sample_rate_ == 0) {
            throw NotSupportResampleSampleRateException();
        }
        converter_->Start(input_format_.GetSampleRate(),
            input_format_.GetChannels(),
            target_sample_rate_);
    }

    XAMP_LOG_D(logger_, "Output device format: {} num_read_sample: {} resampler: {} buffer: {}.",
        output_format_,
        num_read_sample_,
        converter_->GetDescription(),
        String::FormatBytes(fifo_.GetSize()));
}

bool AudioPlayer::IsEnableSampleRateConverter() const {
    return enable_sample_converter_;
}

void AudioPlayer::SetDeviceFormat() {
    input_format_ = stream_->GetFormat();

    if (IsEnableSampleRateConverter() && CanProcessFile()) {
        if (output_format_.GetSampleRate() != target_sample_rate_) {
            device_id_.clear();
        }
        output_format_ = input_format_;
        output_format_.SetSampleRate(target_sample_rate_);
    }
    else {
        if (input_format_.GetSampleRate() != output_format_.GetSampleRate()) {
            device_id_.clear();
        }
        output_format_ = input_format_;
    }

	for (auto &dsp : dsp_chain_) {
        dsp->SetSampleRate(input_format_.GetSampleRate());
	}
}

void AudioPlayer::OnVolumeChange(float vol) noexcept {
    if (auto adapter = state_adapter_.lock()) {
        adapter->OnVolumeChanged(vol);
        XAMP_LOG_D(logger_, "Volum change: {}.", vol);
    }
}

void AudioPlayer::OnError(const Exception& e) noexcept {
    is_playing_ = false;
    XAMP_LOG_DEBUG(e.what());
}

void AudioPlayer::OnDeviceStateChange(DeviceState state, std::string const & device_id) {    
    if (auto state_adapter = state_adapter_.lock()) {
        switch (state) {
        case DeviceState::DEVICE_STATE_ADDED:
            XAMP_LOG_D(logger_, "Device added device id:{}.", device_id);
            state_adapter->OnDeviceChanged(DeviceState::DEVICE_STATE_ADDED);
            break;
        case DeviceState::DEVICE_STATE_REMOVED:
            XAMP_LOG_D(logger_, "Device removed device id:{}.", device_id);
            if (device_id == device_id_) {
                // TODO: In many system has more ASIO device.
                if (AudioDeviceManager::IsASIODevice(device_type_->GetTypeId())) {
                    AudioDeviceManager::RemoveASIODriver();
                }
                
                state_adapter->OnDeviceChanged(DeviceState::DEVICE_STATE_REMOVED);
                if (device_ != nullptr) {
                    device_->AbortStream();
                    XAMP_LOG_D(logger_, "Device abort stream id:{}.", device_id);
                }
            }
            break;
        case DeviceState::DEVICE_STATE_DEFAULT_DEVICE_CHANGE:
            XAMP_LOG_D(logger_, "Default device device id:{}.", device_id);
            state_adapter->OnDeviceChanged(DeviceState::DEVICE_STATE_DEFAULT_DEVICE_CHANGE);
            break;
        }
    }
}

void AudioPlayer::OpenDevice(double stream_time) {
#ifdef ENABLE_ASIO
    if (auto* dsd_output = audio_util::AsDsdDevice(device_)) {
        if (auto* const dsd_stream = audio_util::AsDsdStream(stream_)) {
            if (dsd_stream->GetDsdMode() == DsdModes::DSD_MODE_NATIVE) {
                dsd_output->SetIoFormat(DsdIoFormat::IO_FORMAT_DSD);
                dsd_mode_ = DsdModes::DSD_MODE_NATIVE;
            }
            else {
                dsd_output->SetIoFormat(DsdIoFormat::IO_FORMAT_PCM);
                dsd_mode_ = DsdModes::DSD_MODE_PCM;
            }
        }
    } else {
        dsd_mode_ = DsdModes::DSD_MODE_PCM;
    }
#endif
    device_->OpenStream(output_format_);
    device_->SetStreamTime(stream_time);
}

void AudioPlayer::BufferStream(double stream_time) {
    fifo_.Clear();

    stream_->Seek(stream_time);

    if (enable_sample_converter_) {
        converter_->Flush();
    }

    sample_size_ = stream_->GetSampleSize();    
    BufferSamples(stream_, converter_, kBufferStreamCount);
}

void AudioPlayer::Startup() {
    if (timer_.IsStarted()) {
        return;
    }
    
    constexpr size_t kWorkingSetSize = 2048ul * 1024ul * 1024ul;
    device_manager_.SetWorkingSetSize(kWorkingSetSize);

    device_manager_.RegisterDeviceListener(shared_from_this());
    wait_timer_.SetTimeout(kReadSampleWaitTime);

    std::weak_ptr<AudioPlayer> player = shared_from_this();
    timer_.Start(kUpdateSampleInterval, [player]() {
        auto p = player.lock();
        if (!p) {
            return;
        }

        auto adapter = p->state_adapter_.lock();
        if (!adapter) {
            return;
        }

        if (p->is_paused_) {
            return;
        }
        if (!p->is_playing_) {
            return;
        }

        const auto slice = p->slice_.load();
        if (slice.sample_size > 0) {
            adapter->OnSampleTime(slice.stream_time);            
        }
        else if (p->is_playing_ && slice.sample_size == -1) {
            p->SetState(PlayerState::PLAYER_STATE_STOPPED);
            p->is_playing_ = false;
        }
        });
}

void AudioPlayer::Seek(double stream_time) {
    if (!device_) {
        return;
    }

    if (device_->IsStreamOpen()) {
        seek_queue_.TryPush(stream_time);
    }
}

void AudioPlayer::DoSeek(double stream_time) {
	if (state_ != PlayerState::PLAYER_STATE_PAUSED) {
        Pause();
	}
	
    try {
        stream_->Seek(stream_time);
    }
    catch (std::exception const& e) {
        XAMP_LOG_DEBUG(e.what());
        Resume();
        return;
    }
	
    device_->SetStreamTime(stream_time);
    sample_end_time_ = stream_->GetDuration() - stream_time;
    XAMP_LOG_D(logger_, "Player duration:{} seeking:{} sec, end time:{} sec.",
        stream_->GetDuration(),
        stream_time,
        sample_end_time_);
    UpdateSlice(0, stream_time);
    fifo_.Clear();
    BufferStream(stream_time);
    Resume();
}

AudioPlayer::AudioSlice::AudioSlice(int32_t const sample_size, double const stream_time) noexcept
	: sample_size(sample_size)
	, stream_time(stream_time) {
}

void AudioPlayer::BufferSamples(AlignPtr<FileStream>& stream, AlignPtr<SampleRateConverter>& converter, int32_t buffer_count) {
    InitProcessor();
	
    auto* const sample_buffer = read_buffer_.Get();

    for (auto i = 0; i < buffer_count; ++i) {
        while (true) {
            const auto num_samples = stream->GetSamples(sample_buffer, num_read_sample_);
            if (num_samples == 0) {
                return;
            }

            auto* samples = reinterpret_cast<float*>(sample_buffer);

        	if (CanProcessFile() && !dsp_chain_.empty() && IsEnableProcessor()) {
                auto itr = dsp_chain_.begin();
                if (!converter->Process((*itr)->Process(samples, num_samples), fifo_)) {
                    continue;
                }
        	} else {
                if (!converter->Process(samples, num_samples, fifo_)) {
                    continue;
                }
        	}
            break;
        }
    }
}

bool AudioPlayer::CanProcessFile() const noexcept {
    return (dsd_mode_ == DsdModes::DSD_MODE_PCM || dsd_mode_ == DsdModes::DSD_MODE_DSD2PCM);
}

void AudioPlayer::ReadSampleLoop(int8_t *sample_buffer, uint32_t max_buffer_sample, std::unique_lock<FastMutex>& lock) {
    while (is_playing_) {
        const auto num_samples = stream_->GetSamples(sample_buffer, max_buffer_sample);

        if (num_samples > 0) {
            auto *samples = reinterpret_cast<float*>(sample_buffer);

            if (CanProcessFile() && !dsp_chain_.empty() && IsEnableProcessor()) {
                auto itr = dsp_chain_.begin();
                if (!converter_->Process((*itr)->Process(samples, num_samples), fifo_)) {
                    continue;
                }
            }
            else {
                if (!converter_->Process(samples, num_samples, fifo_)) {
                    continue;
                }
            }
        } else {
            wait_timer_.Wait();
        }
        break;
    }
}

AlignPtr<SampleRateConverter> AudioPlayer::CloneSampleRateConverter() const {
    return converter_->Clone();
}

AudioDeviceManager& AudioPlayer::GetAudioDeviceManager() {
    return device_manager_;
}

void AudioPlayer::Play() {
    if (!device_) {
        return;
    }

    is_playing_ = true;
    if (device_->IsStreamOpen()) {
        if (!device_->IsStreamRunning()) {
            device_->SetVolume(volume_);
            device_->SetMute(is_muted_);
            XAMP_LOG_D(logger_, "Play vol:{} muted:{}.", volume_, is_muted_);
#ifdef _DEBUG
            sw_.Reset();
#endif
            device_->StartStream();
            SetState(PlayerState::PLAYER_STATE_RUNNING);
        }
    }

    if (stream_task_.valid()) {
        return;
    }

    stream_task_ = ThreadPool::GetInstance().Spawn([player = shared_from_this()]() noexcept {
        auto* p = player.get();

        std::unique_lock lock{ p->pause_mutex_ };

        auto* sample_buffer = p->read_buffer_.Get();
        const auto max_buffer_sample = p->num_read_sample_;
        const auto num_sample_write = max_buffer_sample * kMaxWriteRatio;

        XAMP_LOG_D(p->logger_, "max_buffer_sample: {}, num_sample_write: {}", max_buffer_sample, num_sample_write);

        try {
            while (p->is_playing_) {
                while (p->is_paused_) {
                    p->pause_cond_.wait_for(lock, kPauseWaitTimeout);
                    p->ProcessEvent();
                }

                p->ProcessEvent();

                if (p->fifo_.GetAvailableWrite() < num_sample_write) {
                    p->wait_timer_.Wait();
                    continue;
                }

                p->ReadSampleLoop(sample_buffer, max_buffer_sample, lock);
            }
        }
        catch (const Exception& e) {
            XAMP_LOG_DEBUG("Stream thread read has exception: {}", e.what());
        }
        catch (const std::exception& e) {
            XAMP_LOG_DEBUG("Stream thread read has exception: {}", e.what());
        }

        XAMP_LOG_D(p->logger_, "Stream thread done!");
        p->stream_.reset();
    });
}

#ifdef _DEBUG
void AudioPlayer::CheckRace() {
    std::lock_guard<FastMutex> guard{ debug_mutex_ };
    auto current_thread = GetCurrentThreadId();
    if (render_thread_id_ != current_thread) {
        XAMP_LOG_INFO("********* Render thread was change : {} => {}", render_thread_id_, current_thread);
    }
    render_thread_id_ = current_thread;
}
#endif

DataCallbackResult AudioPlayer::OnGetSamples(void* samples, size_t num_buffer_frames, double stream_time, double sample_time) noexcept {
#ifdef _DEBUG
    CheckRace();
#endif
	
    const auto num_samples = num_buffer_frames * output_format_.GetChannels();
    const auto sample_size = num_samples * sample_size_;
#ifdef _DEBUG
    const auto elapsed = sw_.Elapsed();
    max_process_time_ = std::max(elapsed, max_process_time_);
#endif

    XAMP_LIKELY(fifo_.TryRead(static_cast<int8_t*>(samples), sample_size)) {       
        UpdateSlice(static_cast<int32_t>(num_samples), stream_time);
#ifdef _DEBUG
        sw_.Reset();
#endif        
        return DataCallbackResult::CONTINUE;
    }

    if (sample_time <= sample_end_time_) {
        MemorySet(static_cast<int8_t*>(samples), 0, sample_size);
        return DataCallbackResult::CONTINUE;
    }

    UpdateSlice(-1, stream_time);
    return DataCallbackResult::STOP;
}

void AudioPlayer::PrepareToPlay() {
    SetDeviceFormat();
    CreateDevice(device_info_.device_type_id, device_info_.device_id, false);
    OpenDevice(0);
    CreateBuffer();
    BufferStream(0);    
    
    sample_end_time_ = stream_->GetDuration();

    XAMP_LOG_D(logger_, "Stream end time {} sec", sample_end_time_);
}

}
