#include <base/str_utilts.h>
#include <base/platform.h>
#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/stl.h>
#include <base/threadpoolexecutor.h>
#include <base/dsdsampleformat.h>
#include <base/buffer.h>
#include <base/timer.h>
#include <base/scopeguard.h>
#include <base/waitabletimer.h>
#include <base/stopwatch.h>
#include <base/executor.h>
#include <base/trackinfo.h>

#include <output_device/api.h>
#include <output_device/win32/asiodevicetype.h>
#include <output_device/idsddevice.h>
#include <output_device/iaudiodevicemanager.h>

#include <stream/api.h>
#include <stream/dspmanager.h>
#include <stream/iaudiostream.h>
#include <stream/idsdstream.h>
#include <stream/filestream.h>
#include <stream/bassfader.h>
#include <stream/iaudioprocessor.h>
#include <stream/avfilestream.h>
#include <stream/bassfilestream.h>
#include <stream/r8brainresampler.h>
#include <stream/compressorconfig.h>
#include <stream/basscompressor.h>

#include <player/iplaybackstateadapter.h>
#include <player/audio_player.h>

XAMP_AUDIO_PLAYER_NAMESPACE_BEGIN
namespace {
    XAMP_DECLARE_LOG_NAME(AudioPlayer);

    constexpr int32_t kBufferStreamCount = 8;
    // 32MB
    constexpr uint32_t kPreallocateBufferSize = 32 * 1024 * 1024;
    // 128MB
    constexpr uint32_t kMaxPreAllocateBufferSize = 128 * 1024 * 1024;
    // 8KB
    constexpr uint32_t kMinReadBufferSize = 8192;

    constexpr int32_t  kTotalBufferStreamCount = 32;
    constexpr uint32_t kMaxWriteRatio = 20;
    constexpr uint32_t kMaxReadRatio = 4;
    constexpr uint32_t kMaxBufferSecs = 5;
    constexpr uint32_t kActionQueueSize = 30;

    constexpr std::chrono::milliseconds kUpdateSampleIntervalMs(100);
    constexpr std::chrono::milliseconds kReadSampleWaitTimeMs(15);
    constexpr std::chrono::milliseconds kPauseWaitTimeout(30);
    constexpr std::chrono::seconds kWaitForStreamStopTime(10);
    constexpr std::chrono::seconds kWaitForSignalWhenReadFinish(3);
    constexpr std::chrono::milliseconds kMinimalCopySamplesTime(5);    

    int32_t GetBufferCount(int32_t sample_rate) {
        return sample_rate > (176400 * 2) ? kBufferStreamCount : 3;
    }

#if defined(XAMP_OS_WIN)
    IDsdDevice* AsDsdDevice(AlignPtr<IOutputDevice> const& device) noexcept {
        return dynamic_cast<IDsdDevice*>(device.get());
    }
#endif
}

AudioPlayer::AudioPlayer()
    : is_muted_(false)
	, is_dsd_file_(false)
    , enable_fadeout_(true)
	, enable_file_cache_(true)
    , dsd_mode_(DsdModes::DSD_MODE_PCM)
    , sample_size_(0)
    , target_sample_rate_(0)
    , num_read_buffer_size_(0)
    , num_write_buffer_size_(0)
    , volume_(0)
	, stream_offset_time_(0)
    , is_fade_out_(false)
    , is_playing_(false)
    , is_paused_(false)
    , stream_time_sec_unit_{-1}
    , state_(PlayerState::PLAYER_STATE_STOPPED)
    , sample_end_time_(0)
    , stream_duration_(0)
    , dsp_manager_(StreamFactory::MakeDSPManager())
    , device_manager_(MakeAudioDeviceManager())
    , logger_(XampLoggerFactory.GetLogger(kAudioPlayerLoggerName))
    , action_queue_(kActionQueueSize)
	, fifo_(AlignUp(kPreallocateBufferSize, GetPageSize()))
    , thread_pool_(ThreadPoolBuilder::MakePlaybackThreadPool()) {
    PreventSleep(true);
}

AudioPlayer::~AudioPlayer() {
    Destroy();
}

void AudioPlayer::Destroy() {
    timer_.Stop();
    try {
        CloseDevice(true, true);
    }
    catch (...) {
    }
    Stop(false, true);
    stream_.reset();
    read_buffer_.reset();
#if defined(XAMP_OS_WIN)
    ResetAsioDriver();
#endif

    PreventSleep(false);
    FreeAvLib();

    device_.reset();
    device_manager_.reset();
}

void AudioPlayer::SetStateAdapter(const std::weak_ptr<IPlaybackStateAdapter>& adapter) {
    state_adapter_ = adapter;
    device_manager_->RegisterDeviceListener(shared_from_this());

    std::weak_ptr<AudioPlayer> player = shared_from_this();
    timer_.Start(kUpdateSampleIntervalMs, [player]() {
        auto p = player.lock();
        if (!p) {
            return;
        }

        const auto adapter = p->state_adapter_.lock();
        if (!adapter) {
            return;
        }

        if (p->is_paused_) {
            return;
        }
        if (!p->is_playing_) {
            return;
        }

        const auto stream_time_sec_unit = p->stream_time_sec_unit_.load();
        if (stream_time_sec_unit > 0) {
            adapter->OnSampleTime(stream_time_sec_unit / 1000.0);
        }
        else if (p->is_playing_ && stream_time_sec_unit == -1) {
            p->SetState(PlayerState::PLAYER_STATE_STOPPED);
            p->is_playing_ = false;
        }
    });
}

void AudioPlayer::Open(const Path& file_path, const Uuid& device_id) {
    AlignPtr<IDeviceType> device_type;
    if (device_id.IsValid()) {
        device_type = device_manager_->CreateDefaultDeviceType();
    } else {
        device_type = device_manager_->Create(device_id);
    }

    device_type->ScanNewDevice();
    if (const auto device_info = device_type->GetDefaultDeviceInfo()) {
        Open(file_path, *device_info);
    } else {
        throw DeviceNotFoundException();
    }
}

void AudioPlayer::Open(const Path& file_path, const DeviceInfo& device_info, uint32_t target_sample_rate, DsdModes output_mode) {
    CloseDevice(true);
    UpdatePlayerStreamTime();
    OpenStream(file_path, output_mode);
    device_info_ = device_info;
    target_sample_rate_ = target_sample_rate;
}

void AudioPlayer::CreateDevice(const Uuid& device_type_id, const  std::string & device_id, bool open_always) {
    if (device_ == nullptr
        || device_id_ != device_id
        || device_type_id_ != device_type_id
        || open_always) {
        if (device_type_id_ != device_type_id) {
            // device可能是ASIO解後再移除driver.
            device_.reset();
            ResetAsioDriver();
            XAMP_LOG_D(logger_, "ResetASIODriver!");
        }    	
        device_type_ = device_manager_->Create(device_type_id);
        device_type_->ScanNewDevice();
        device_ = device_type_->MakeDevice(device_id);
        device_type_id_ = device_type_id;
        device_id_ = device_id;
        XAMP_LOG_D(logger_, "Create device: {}", device_type_->GetDescription());
    }
    device_->SetAudioCallback(this);
}

bool AudioPlayer::IsDsdFile() const {
    return is_dsd_file_;
}

void AudioPlayer::ReadStreamInfo(DsdModes dsd_mode, AlignPtr<FileStream>& stream) {
    dsd_mode_ = dsd_mode;

    stream_duration_ = stream->GetDurationAsSeconds();
    input_format_ = stream->GetFormat();

    if (dsd_mode == DsdModes::DSD_MODE_PCM) {
        dsd_speed_ = std::nullopt;
        is_dsd_file_ = false;
        return;
    }

    if (const auto* dsd_stream = AsDsdStream(stream)) {
        if (!dsd_stream->IsDsdFile()) {
            return;
        }
        is_dsd_file_ = true;
        dsd_speed_ = dsd_stream->GetDsdSpeed();
    } else {
        is_dsd_file_ = false;
        dsd_speed_ = std::nullopt;
    }
}

void AudioPlayer::OpenStream(const Path & file_path, DsdModes dsd_mode) {
    static const std::string kApeFileExtention = ".ape";

    stream_ = MakeFileStream(file_path, dsd_mode);

    if (file_path.extension() != kApeFileExtention) {
        for (auto i = 0; i < 1; ++i) {
            try
            {
                stream_->OpenFile(file_path);
            }
            catch (const Exception& e) {
                // Fallback other stream
                if (stream_->GetTypeId() == XAMP_UUID_OF(AvFileStream)) {
                    stream_ = MakeAlign<FileStream, BassFileStream>();
                }
                else {
                    stream_ = MakeAlign<FileStream, AvFileStream>();
                }
                stream_->OpenFile(file_path);
                XAMP_LOG_E(logger_, "{}", e.what());
            }
        }
    }
    else {
        stream_ = MakeAlign<FileStream, BassFileStream>();
        stream_->OpenFile(file_path);
    }    

    ReadStreamInfo(dsd_mode, stream_);
    XAMP_LOG_D(logger_, "Open stream type: {} {} duration:{:.2f} sec.",
        stream_->GetDescription(),
        dsd_mode_,
        stream_duration_.load());
}

void AudioPlayer::SetState(const PlayerState play_state) {
    if (const auto adapter = state_adapter_.lock()) {
        adapter->OnStateChanged(play_state);
    }
    state_ = play_state;
    XAMP_LOG_D(logger_, "Set state: {}.", EnumToString(state_));
}

void AudioPlayer::ReadPlayerAction() {
    while (!action_queue_.empty()) {
        PlayerAction msg;
        if (action_queue_.TryDequeue(msg)) {
            try {
                switch (msg.id) {
                case PlayerActionId::PLAYER_SEEK:
                {
                    auto stream_time = std::any_cast<double>(msg.content);
                    XAMP_LOG_D(logger_, "Receive seek {:.2f} message.", stream_time);
                    DoSeek(stream_time);
                }
                break;
                }
            }
            catch (const std::exception& e) {
                XAMP_LOG_D(logger_, "Receive {} {}.", EnumToString(msg.id), e.what());
            }
        }
    }
}
	
void AudioPlayer::Pause() {
    if (!device_) {
        return;
    }

    XAMP_LOG_D(logger_, "Player pause.");
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
        read_finish_and_wait_seek_signal_cond_.notify_all();
        device_->StartStream();
        SetState(PlayerState::PLAYER_STATE_RUNNING);
    }
}

void AudioPlayer::FadeOut() {
    const auto sample_count = output_format_.GetSecondsSize(kFadeTimeSeconds) / sizeof(float);

    Buffer<float> buffer(sample_count);
    size_t num_filled_count = 0;
    dynamic_cast<BassFader*>(fader_.get())->SetTime(1.0f, 0.0f, kFadeTimeSeconds);

    if (!fifo_.TryRead(reinterpret_cast<int8_t*>(buffer.data()), buffer.GetByteSize(), num_filled_count)) {
        return;
    }

    Buffer<float> fade_buf(sample_count);
    BufferRef<float> buf_ref(fade_buf);
    if (!fader_->Process(buffer.data(), buffer.size(), buf_ref)) {
        XAMP_LOG_W(logger_, "Fade out audio process failure!");
    }

    fifo_.Clear();
    fifo_.TryWrite(reinterpret_cast<int8_t*>(buf_ref.data()), buf_ref.GetByteSize());
}

void AudioPlayer::ProcessFadeOut() {
    if (!device_) {
        return;
    }
    if (dsd_mode_ == DsdModes::DSD_MODE_PCM
        || dsd_mode_ == DsdModes::DSD_MODE_DSD2PCM) {
        XAMP_LOG_D(logger_, "Process fadeout.");
        is_fade_out_ = true;
        delay_callback_(kFadeTimeSeconds);
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
        UpdatePlayerStreamTime();
        if (signal_to_stop) {
            SetState(PlayerState::PLAYER_STATE_USER_STOPPED);                        
        }
    }

    if (shutdown_device) {
        XAMP_LOG_D(logger_, "Shutdown device.");
        if (IsAsioDevice(device_type_id_)) {
            device_.reset();
            ResetAsioDriver();           
        }
        device_id_.clear();
        device_.reset();
    }
    stream_.reset();
    fifo_.Clear();
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
    return false;
}

bool AudioPlayer::IsMute() const {
    return is_muted_;
}

void AudioPlayer::SetMute(bool mute) {
    is_muted_ = mute;
    if (!device_ || !device_->IsStreamOpen()) {
        return;
    }
    device_->SetMute(mute);
}

std::optional<uint32_t> AudioPlayer::GetDsdSpeed() const {
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
    auto file_format = input_format_;
    file_format.SetBitPerSample(stream_->GetBitDepth());
    return file_format;
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

void AudioPlayer::CloseDevice(bool wait_for_stop_stream, bool quit) {
    is_playing_ = false;
    is_paused_ = false;
    pause_cond_.notify_all();
    read_finish_and_wait_seek_signal_cond_.notify_all();

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
        stream_task_ = Task<void>();
        XAMP_LOG_D(logger_, "Stream thread was finished.");
    }

    stream_offset_time_ = 0;

    if (!quit && enable_fadeout_) {
        fader_ = StreamFactory::MakeFader();
        fader_->Initialize(config_);
        ProcessFadeOut();
    }

    if (device_ != nullptr) {
        if (device_->IsStreamOpen()) {
            XAMP_LOG_D(logger_, "Stop output device");
            try {
                device_->StopStream(wait_for_stop_stream);
                device_->CloseStream();
			}
			catch (const Exception& e) {
				XAMP_LOG_D(logger_, "Close device failure. {}", e.what());
			}
		}
	}

    fifo_.Clear();
    action_queue_.clear();
}

void AudioPlayer::ResizeReadBuffer(uint32_t allocate_size) {
    if (read_buffer_.GetSize() == 0 || read_buffer_.GetSize() != allocate_size) {
        XAMP_LOG_D(logger_, "Allocate read buffer : {}.", String::FormatBytes(allocate_size));
        read_buffer_ = MakeBuffer<int8_t>(allocate_size);
    }
}

void AudioPlayer::ResizeFIFO(uint32_t fifo_size) {
    if (fifo_.GetSize() == 0 || fifo_.GetSize() < fifo_size) {
        XAMP_LOG_D(logger_, "Allocate fifo buffer : {}.", String::FormatBytes(fifo_size));
        fifo_.Resize(fifo_size);
    }
}

void AudioPlayer::CreateBuffer() {
    uint32_t allocate_size = 0;
    uint32_t fifo_size = 0;

    auto get_buffer_sample = [](auto *device, auto ratio) {
        return static_cast<uint32_t>(AlignUp(device->GetBufferSize() * ratio, GetPageSize()));
    };

    if (dsd_mode_ == DsdModes::DSD_MODE_NATIVE) {
        // DSD native mode output buffer size is 8 times of the sample rate.
        num_read_buffer_size_ = static_cast<uint32_t>(AlignUp(output_format_.GetSampleRate() / 8), GetPageSize());
        // FIFO buffer size is 5 seconds of the output format.
        fifo_size = output_format_.GetAvgBytesPerSec() * kMaxBufferSecs * output_format_.GetSampleSize();
        allocate_size = num_read_buffer_size_;
        num_write_buffer_size_ = device_->GetBufferSize() * kMaxBufferSecs;
    }
    else {
        // Note: 如果比讀取的緩衝區還要小的話就沒辦法正確撥放.
        // Calculate the buffer size from the output format.
        auto max_ratio = (std::max)(output_format_.GetAvgBytesPerSec() / input_format_.GetAvgBytesPerSec(), 1U);
        num_write_buffer_size_ = get_buffer_sample(device_.get(), max_ratio * sizeof(float));        
        num_read_buffer_size_ = (std::max)(get_buffer_sample(device_.get(), kMaxReadRatio), kMinReadBufferSize);   
        // Calculate the buffer size from the stream.
        allocate_size = std::min(kMaxPreAllocateBufferSize,
            num_write_buffer_size_
            * stream_->GetSampleSize() 
            * kTotalBufferStreamCount);
        // Align up the buffer size.
        allocate_size = AlignUp(allocate_size, GetPageSize());
        if (!enable_file_cache_) {
            fifo_size = kMaxBufferSecs * output_format_.GetAvgBytesPerSec() * GetBufferCount(output_format_.GetSampleRate());
        } else {
            fifo_size = kMaxPreAllocateBufferSize;
        }
        allocate_size = AlignUp(fifo_size, GetPageSize());
    }

    ResizeReadBuffer(allocate_size);
    ResizeFIFO(fifo_size);

    XAMP_LOG_DEBUG("Device output buffer:{} num_write_buffer_size:{} num_read_sample:{} fifo buffer:{}.",
        String::FormatBytes(device_->GetBufferSize()),
        String::FormatBytes(num_write_buffer_size_),
        String::FormatBytes(num_read_buffer_size_),
        String::FormatBytes(fifo_size));
}

void AudioPlayer::SetDeviceFormat() {
    if (target_sample_rate_ != 0 && IsPcmAudio()) {
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
}

void AudioPlayer::OnVolumeChange(int32_t vol) noexcept {
    if (const auto adapter = state_adapter_.lock()) {
        adapter->OnVolumeChanged(vol);
        XAMP_LOG_D(logger_, "Volume change: {}.", vol);
    }
}

void AudioPlayer::OnError(const std::exception& e) noexcept {
    is_playing_ = false;
    if (const auto adapter = state_adapter_.lock()) {
        adapter->OnError(e);
    }
}

void AudioPlayer::OnDeviceStateChange(DeviceState state, const std::string & device_id) {    
    if (const auto state_adapter = state_adapter_.lock()) {
        switch (state) {
        case DeviceState::DEVICE_STATE_ADDED:
            XAMP_LOG_D(logger_, "Device added device id:{}.", device_id);
            state_adapter->OnDeviceChanged(DeviceState::DEVICE_STATE_ADDED, device_id);
            break;
        case DeviceState::DEVICE_STATE_REMOVED:
            XAMP_LOG_D(logger_, "Device removed device id:{}.", device_id);
            if (device_id == device_id_) {
                // TODO: In many system has more ASIO device.
                if (IsAsioDevice(device_type_->GetTypeId())) {
                    ResetAsioDriver();
                }
                
                state_adapter->OnDeviceChanged(DeviceState::DEVICE_STATE_REMOVED, device_id);
                if (device_ != nullptr) {
                    device_->AbortStream();
                    XAMP_LOG_D(logger_, "Device abort stream id:{}.", device_id);
                }
            }
            break;
        case DeviceState::DEVICE_STATE_DEFAULT_DEVICE_CHANGE:
            XAMP_LOG_D(logger_, "Default device device id:{}.", device_id);
            state_adapter->OnDeviceChanged(DeviceState::DEVICE_STATE_DEFAULT_DEVICE_CHANGE, device_id);
            break;
        }
    }
}

void AudioPlayer::OpenDevice(double stream_time) {
#if defined(XAMP_OS_WIN)
    if (auto* dsd_output = AsDsdDevice(device_)) {
        if (dsd_mode_ == DsdModes::DSD_MODE_AUTO 
            || dsd_mode_ == DsdModes::DSD_MODE_PCM 
            || dsd_mode_ == DsdModes::DSD_MODE_DOP) {
            if (const auto* const dsd_stream = AsDsdStream(stream_)) {
                if (dsd_mode_ == DsdModes::DSD_MODE_NATIVE) {
                    dsd_output->SetIoFormat(DsdIoFormat::IO_FORMAT_DSD);
                }
                else {
                    if (dsd_mode_ == DsdModes::DSD_MODE_DOP) {
                        dsd_output->SetIoFormat(DsdIoFormat::IO_FORMAT_DOP);
                    } else {
                        dsd_output->SetIoFormat(DsdIoFormat::IO_FORMAT_PCM);
                    }
                }
            }            
        } else {
            output_format_.SetFormat(DataFormat::FORMAT_DSD);
            output_format_.SetByteFormat(ByteFormat::SINT8);
            dsd_output->SetIoFormat(DsdIoFormat::IO_FORMAT_DSD);
        }
    }
#endif
    device_->OpenStream(output_format_);
    device_->SetStreamTime(stream_time);
}

void AudioPlayer::BufferStream(double stream_time, const std::optional<double>& offset, const std::optional<double>& duration) {
    XAMP_LOG_D(logger_, "Buffing samples : {:.2f}ms", stream_time);

	if (dsp_manager_->Contains(XAMP_UUID_OF(R8brainSampleRateConverter))) {
        SetReadSampleSize(kR8brainBufferSize);
	}

    if (offset.has_value()) {
		stream_offset_time_ = offset.value();
    }

    if (duration.has_value()) {
		stream_duration_ = duration.value();
    }

    fifo_.Clear();
    stream_->SeekAsSeconds(stream_offset_time_ + stream_time);
    sample_size_ = stream_->GetSampleSize();    
    BufferSamples(stream_, GetBufferCount(output_format_.GetSampleRate()));
}

void AudioPlayer::EnableFadeOut(bool enable) {
    enable_fadeout_ = enable;
}

void AudioPlayer::UpdatePlayerStreamTime(int32_t stream_time_sec_unit) noexcept {
    stream_time_sec_unit_.exchange(stream_time_sec_unit);
}

void AudioPlayer::Seek(double stream_time) {
    if (!device_) {
        return;
    }

    if (device_->IsStreamOpen()) {
        read_finish_and_wait_seek_signal_cond_.notify_one();
        PlayerAction action{
            PlayerActionId::PLAYER_SEEK,
            stream_time
        };
        action_queue_.TryEnqueue(std::move(action));
    }
}

void AudioPlayer::DoSeek(double stream_time) {
	if (state_ != PlayerState::PLAYER_STATE_PAUSED) {
        Pause();
	}
	
    try {
        stream_->SeekAsSeconds(stream_time);
    }
    catch (const Exception& e) {
        XAMP_LOG_D(logger_, e.GetErrorMessage());
        Resume();
        return;
    }

    device_->SetStreamTime(stream_time);
    sample_end_time_ = stream_->GetDurationAsSeconds() - stream_time;
    XAMP_LOG_D(logger_, "Stream duration:{:.2f} seeking:{:.2f} sec, end time:{:.2f} sec.",
        stream_->GetDurationAsSeconds(),
        stream_time,
        sample_end_time_);
    UpdatePlayerStreamTime(stream_time * 1000);
    fifo_.Clear();
    BufferStream(stream_time);
    Resume();
}

bool AudioPlayer::IsPcmAudio() const noexcept {
    return (dsd_mode_ == DsdModes::DSD_MODE_PCM || dsd_mode_ == DsdModes::DSD_MODE_DSD2PCM);
}

void AudioPlayer::BufferSamples(const AlignPtr<FileStream>& stream, int32_t buffer_count) {
    auto* const sample_buffer = read_buffer_.Get();

    for (auto i = 0; i < buffer_count && stream_->IsActive(); ++i) {
        XAMP_LOG_D(logger_, "Buffering {} ...", i);

        while (true) {
            const auto num_samples = stream->GetSamples(sample_buffer, num_read_buffer_size_);
            if (num_samples == 0) {
                return;
            }

            auto* samples = reinterpret_cast<const float*>(sample_buffer);
            if (dsp_manager_->ProcessDSP(samples, num_samples, fifo_)) {
                continue;
            }            
            break;
        }
    }
}

const AlignPtr<IAudioDeviceManager>& AudioPlayer::GetAudioDeviceManager() {
    return device_manager_;
}

AlignPtr<IDSPManager>& AudioPlayer::GetDspManager() {
    return dsp_manager_;
}

void AudioPlayer::SetReadSampleSize(uint32_t num_samples) {
    num_read_buffer_size_ = num_samples;
    XAMP_LOG_D(logger_, "Output buffer:{} device format: {} num_read_sample: {} fifo buffer: {}.",
               device_->GetBufferSize(),
               output_format_,
               String::FormatBytes(num_read_buffer_size_),
               String::FormatBytes(fifo_.GetSize()));
}

void AudioPlayer::WaitForReadFinishAndSeekSignal(std::unique_lock<FastMutex>& stopped_lock) {
    if (read_finish_and_wait_seek_signal_cond_.wait_for(stopped_lock, kWaitForSignalWhenReadFinish) != std::cv_status::timeout) {
        XAMP_LOG_D(logger_, "Stream is read done!, Weak up for seek signal.");
    }
}

bool AudioPlayer::ShouldKeepReading() const noexcept {
    return is_playing_ && stream_->IsActive();
}

void AudioPlayer::ReadSampleLoop(int8_t* buffer, uint32_t buffer_size, std::unique_lock<FastMutex>& stopped_lock) {
    if (!stream_->IsActive()) {
        if (is_playing_) {
            WaitForReadFinishAndSeekSignal(stopped_lock);
        }
        return;
    }    

    while (ShouldKeepReading()) {
        const auto num_samples = stream_->GetSamples(buffer, buffer_size);

        if (num_samples > 0) {
            auto* samples = reinterpret_cast<float*>(buffer);
            
            if (dsp_manager_->ProcessDSP(samples, num_samples, fifo_)) {
                continue;
            }
        }
        if (!enable_file_cache_ || !IsAvailableWrite()) {
            break;
        }        
    }
}

bool AudioPlayer::IsAvailableWrite() const noexcept {
    const auto num_write_buffer_size = num_write_buffer_size_ * kMaxWriteRatio;
    return fifo_.GetAvailableWrite() >= num_write_buffer_size;
}

void AudioPlayer::Play() {
    if (!device_) {
        return;
    }

    if (const auto state = state_adapter_.lock()) {
        state->OutputFormatChanged(output_format_, device_->GetBufferSize());
    }

    is_fade_out_ = false;
    is_playing_ = true;
    if (device_->IsStreamOpen()) {
        if (!device_->IsStreamRunning()) {
            XAMP_LOG_D(logger_, "Play volume:{} muted:{}.", volume_, is_muted_);
            device_->StartStream();
            SetState(PlayerState::PLAYER_STATE_RUNNING);
        }
    }

    if (stream_task_.valid()) {
        return;
    }

    stream_task_ = Executor::Spawn(*thread_pool_, [player = shared_from_this()](const StopToken& stop_token) {
        auto* p = player.get();

        std::unique_lock<FastMutex> pause_lock{ p->pause_mutex_ };
        std::unique_lock<FastMutex> stopped_lock{ p->stopped_mutex_ };

        auto* buffer = p->read_buffer_.Get();
        const auto num_read_buffer_size = p->num_read_buffer_size_;
        const auto num_write_buffer_size = p->num_write_buffer_size_ * kMaxWriteRatio;

        XAMP_LOG_DEBUG("num_read_buffer_size: {}, num_write_buffer_size: {}",
            String::FormatBytes(num_read_buffer_size),
            String::FormatBytes(num_write_buffer_size)
        );

        WaitableTimer wait_timer;
        wait_timer.SetTimeout(kReadSampleWaitTimeMs);

        try {
            while (p->is_playing_ && !stop_token.stop_requested()) {
                // Wait for pause signal.
                while (p->is_paused_) {
                    p->pause_cond_.wait_for(pause_lock, kPauseWaitTimeout);
                    p->ReadPlayerAction();
                }

                // Read action queue.
                p->ReadPlayerAction();

                // Check stream is active.
                if (!p->IsAvailableWrite()) {
                    // Wait for next available write time.
                    wait_timer.Wait();
                    XAMP_LOG_T(p->logger_, "FIFO buffer: {} num_sample_write: {}",
                        p->fifo_.GetAvailableWrite(),
                        num_write_buffer_size
                    );
                    continue;
                }
                p->ReadSampleLoop(buffer, num_read_buffer_size, stopped_lock);
            }
        }
        catch (const std::exception& e) {
            XAMP_LOG_D(p->logger_, "Stream thread read has exception: {}.", e.what());
            p->OnError(e);
        }
        catch (...) {
            p->OnError(std::exception());
        }

        XAMP_LOG_D(p->logger_, "Stream thread done!");
        p->stream_.reset();
    }, ExecuteFlags::EXECUTE_LONG_RUNNING);
}

void AudioPlayer::CopySamples(void* samples, size_t num_buffer_frames) const {
    if (!IsPcmAudio()) {
        return;
    }

    const auto adapter = state_adapter_.lock();
    if (!adapter) {
        return;
    }

    Stopwatch watch;    
    watch.Reset();    
    
    adapter->OnSamplesChanged(static_cast<const float*>(samples), num_buffer_frames);
    auto elapsed = watch.Elapsed<std::chrono::milliseconds>();
    if (elapsed >= kMinimalCopySamplesTime) {
        XAMP_LOG_D(logger_, "CopySamples too slow ({} ms)!", elapsed.count());
    }
}

DataCallbackResult AudioPlayer::OnGetSamples(void* samples, size_t num_buffer_frames, size_t & num_filled_frames, double stream_time, double /*sample_time*/) noexcept {
    // sample_time: 指的是設備撥放時間, 會被stop時候重置為0.
    // stream time: samples大小累計從開始撥放到結束的時間.
    const auto num_samples = num_buffer_frames * output_format_.GetChannels();
    const auto sample_size = num_samples * sample_size_;

    if (is_fade_out_) {
        FadeOut();
        is_fade_out_ = false;
    }

    if (stream_time >= stream_duration_) {
        UpdatePlayerStreamTime(-1);
        return DataCallbackResult::STOP;
    }

    size_t num_filled_bytes = 0;
    XAMP_LIKELY(fifo_.TryRead(static_cast<int8_t*>(samples), sample_size, num_filled_bytes)) {
        num_filled_frames = num_filled_bytes / sample_size_ / output_format_.GetChannels();
        num_filled_frames = num_buffer_frames;
        UpdatePlayerStreamTime(static_cast<int32_t>(stream_time * 1000));
        CopySamples(samples, num_samples);
        return DataCallbackResult::CONTINUE;
    }

    // note: 為了避免提早將聲音切斷(某些音效介面frames大小,低於某個frame大小就無法撥放),
    // 下次render frame的時候才將聲音停止.
    // 
    // (WASAPI render frame)       (WASAPI render end frame)
    //       |               |                 |
    //       |       (End audio time)          |
    //       V               V                 V
    // <--------------------------------------->
    //
    if (stream_time >= stream_duration_) {
        UpdatePlayerStreamTime(-1);
        return DataCallbackResult::STOP;
    }

    num_filled_frames = num_buffer_frames;
    UpdatePlayerStreamTime(static_cast<int32_t>(stream_time * 1000));
    return DataCallbackResult::CONTINUE;
}

void AudioPlayer::PrepareToPlay(ByteFormat byte_format, uint32_t device_sample_rate) {
    if (device_sample_rate != 0) {
        target_sample_rate_ = device_sample_rate;
    }

    SetDeviceFormat();

	if (byte_format != ByteFormat::INVALID_FORMAT) {
        output_format_.SetByteFormat(byte_format);
    }

    CreateDevice(device_info_.value().device_type_id, device_info_.value().device_id, false);
    OpenDevice(0);
    CreateBuffer();

    config_.AddOrReplace(DspConfig::kInputFormat,  std::any(input_format_));
    config_.AddOrReplace(DspConfig::kOutputFormat, std::any(output_format_));
    config_.AddOrReplace(DspConfig::kDsdMode,      std::any(dsd_mode_));
    config_.AddOrReplace(DspConfig::kSampleSize,   std::any(stream_->GetSampleSize()));

    dsp_manager_->Initialize(config_);
	sample_end_time_ = stream_->GetDurationAsSeconds();
    XAMP_LOG_D(logger_, "Stream end time: {:.2f} sec.", sample_end_time_);    
}

AnyMap& AudioPlayer::GetDspConfig() {
    return config_;
}

void AudioPlayer::SetDelayCallback(const std::function<void(uint32_t)> delay_callback) {
    delay_callback_ = delay_callback;
}

void AudioPlayer::SeFileCacheMode(bool enable) {
    enable_file_cache_ = enable;
}

XAMP_AUDIO_PLAYER_NAMESPACE_END
