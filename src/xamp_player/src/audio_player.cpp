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

    constexpr std::chrono::milliseconds kUpdateSampleIntervalMs(15);
    constexpr std::chrono::milliseconds kReadSampleWaitTimeMs(15);
    constexpr std::chrono::milliseconds kPauseWaitTimeout(30);
    constexpr std::chrono::seconds kWaitForStreamStopTime(10);
    constexpr std::chrono::seconds kWaitForSignalWhenReadFinish(3);
    constexpr std::chrono::milliseconds kMinimalCopySamplesTime(5);    

    int32_t GetBufferCount(int32_t sample_rate) {
        return sample_rate > (176400 * 2) ? kBufferStreamCount : 3;
    }

#if defined(XAMP_OS_WIN)
    IDsdDevice* AsDsdDevice(ScopedPtr<IOutputDevice> const& device) noexcept {
        return dynamic_cast<IDsdDevice*>(device.get());
    }
#endif
}

AudioPlayer::AudioPlayer(
    const std::shared_ptr<IThreadPoolExecutor>& playback_thread_pool,
    const std::shared_ptr<IThreadPoolExecutor>& player_thread_pool)
    : is_muted_(false)
	, is_dsd_file_(false)
    , enable_fadeout_(true)
	, enable_file_cache_(true)
    , num_read_buffer_size_(0)
    , num_write_buffer_size_(0)
    , is_fade_out_(false)
    , sample_end_time_(0)
    , dsp_manager_(StreamFactory::MakeDSPManager())
    , device_manager_(MakeAudioDeviceManager())
    , logger_(XampLoggerFactory.GetLogger(kAudioPlayerLoggerName))
    , action_queue_(kActionQueueSize)
	, fifo_(AlignUp(kPreallocateBufferSize, GetPageSize()))
	, playback_thread_pool_(playback_thread_pool)
	, player_thread_pool_(player_thread_pool) {
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

void AudioPlayer::Open(const Path& file_path, const Uuid& device_id) {
    ScopedPtr<IDeviceType> device_type;
    if (device_id.IsValid()) {
        device_type = device_manager_->CreateDefaultDeviceType();
    } else {
        device_type = device_manager_->Create(device_id);
    }

    device_type->ScanNewDevice();
    if (const auto device_info = 
        device_type->GetDefaultDeviceInfo()) {
        Open(file_path, *device_info);
    } else {
        throw DeviceNotFoundException();
    }
}

void AudioPlayer::Open(const Path& file_path, 
    const DeviceInfo& device_info, 
    uint32_t target_sample_rate, 
    DsdModes output_mode) {
    CloseDevice(true);
    UpdatePlayerStreamTime();
    OpenStream(file_path, output_mode);
    device_info_ = device_info;
    audio_config_.target_sample_rate = target_sample_rate;
}

void AudioPlayer::CreateDevice(const Uuid& device_type_id,
    const std::string & device_id, bool open_always) {
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
        device_ = device_type_->MakeDevice(playback_thread_pool_, device_id);
        device_type_id_ = device_type_id;
        device_id_ = device_id;
        XAMP_LOG_D(logger_, "Create device: {}", device_type_->GetDescription());
    }
    device_->SetAudioCallback(this);
}

bool AudioPlayer::IsDsdFile() const {
    return is_dsd_file_;
}

void AudioPlayer::ReadStreamInfo(DsdModes dsd_mode, 
    const ScopedPtr<FileStream>& stream) {
    audio_config_.dsd_mode = dsd_mode;

    playback_state_.stream_duration = stream->GetDurationAsSeconds();
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
    stream_ = MakeFileStream(file_path, dsd_mode);

    ReadStreamInfo(dsd_mode, stream_);
    XAMP_LOG_D(logger_, "Open stream type: {} {} duration:{:.2f} sec.",
        stream_->GetDescription(),
        audio_config_.dsd_mode,
        playback_state_.stream_duration);
}

void AudioPlayer::SetState(const PlayerState play_state) {
    if (const auto adapter = state_adapter_.lock()) {
        adapter->OnStateChanged(play_state);
    }
    playback_state_.state = play_state;
    XAMP_LOG_D(logger_, "Set state: {}.", EnumToString(playback_state_.state));
}

void AudioPlayer::ReadPlayerAction() {
    PlayerAction msg;
    while (action_queue_.try_dequeue(msg)) {
        try {
            switch (msg.id) {
            case PlayerActionId::PLAYER_SEEK:
            {
                const auto& seek_action = std::get<SeekAction>(msg.content);
                double stream_time = seek_action.stream_time;
                XAMP_LOG_D(logger_, "Receive seek {:.2f} message.", stream_time);
                DoSeek(stream_time);
				playback_state_.is_seeking = false;
            }
            break;

            default:
                XAMP_LOG_D(logger_, 
                    "Unknown action id: {}.", EnumToString(msg.id));
                break;
            }
        }
        catch (const std::bad_variant_access& e) {
            XAMP_LOG_D(logger_, "Failed to get variant content for {}: {}.",
                EnumToString(msg.id), e.what());
        }
        catch (const std::exception& e) {
            XAMP_LOG_D(logger_, "Receive {} {}.", 
                EnumToString(msg.id), e.what());
        }
    }
}
	
void AudioPlayer::Pause() {
    if (!device_) {
        return;
    }

    XAMP_LOG_D(logger_, "Player pause.");
    if (!playback_state_.is_paused) {
        if (device_->IsStreamOpen()) {
            playback_state_.is_paused = true;
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
        playback_state_.is_paused = false;
        pause_cond_.notify_all();
        read_finish_and_wait_seek_signal_cond_.notify_all();
        device_->StartStream();
        SetState(PlayerState::PLAYER_STATE_RUNNING);
    }
}

void AudioPlayer::FadeOut() {
    const auto sample_count =
        output_format_.GetSecondsSize(kFadeTimeSeconds) / sizeof(float);

    Buffer<float> buffer(sample_count);
    size_t num_filled_count = 0;
    dynamic_cast<BassFader*>(fader_.get())->SetTime(1.0f,
        0.0f, kFadeTimeSeconds);

    if (!fifo_.TryRead(reinterpret_cast<std::byte*>(buffer.data()),
        buffer.GetByteSize(),
        num_filled_count)) {
        return;
    }

    Buffer<float> fade_buf(sample_count);
    BufferRef<float> buf_ref(fade_buf);
    if (!fader_->Process(buffer.data(),
        buffer.size(),
        buf_ref)) {
        XAMP_LOG_W(logger_, "Fade out audio process failure!");
    }

    fifo_.Clear();
    fifo_.TryWrite(reinterpret_cast<std::byte*>(buf_ref.data()), 
        buf_ref.GetByteSize());
}

void AudioPlayer::ProcessFadeOut() {
    if (!device_) {
        return;
    }
    if (audio_config_.dsd_mode == DsdModes::DSD_MODE_PCM
        || audio_config_.dsd_mode == DsdModes::DSD_MODE_DSD2PCM) {
        XAMP_LOG_D(logger_, "Process fadeout.");
        is_fade_out_ = true;
        delay_callback_(kFadeTimeSeconds);
    }
}

void AudioPlayer::Stop(bool signal_to_stop,
    bool shutdown_device, 
    bool wait_for_stop_stream) {
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
    audio_config_.volume = volume;
    if (!device_ || !device_->IsStreamOpen()) {
        return;
    }
    device_->SetVolume(volume);
}

uint32_t AudioPlayer::GetVolume() const {
    if (!device_ || !device_->IsStreamOpen()) {
        return audio_config_.volume;
    }
    return device_->GetVolume();
}

bool AudioPlayer::IsHardwareControlVolume() const {
    //if (device_ != nullptr && device_->IsStreamOpen()) {
    //    return device_->IsHardwareControlVolume();
    //}
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
    return playback_state_.stream_duration;
}

PlayerState AudioPlayer::GetState() const noexcept {
    return playback_state_.state;
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
    return playback_state_.is_playing;
}

DsdModes AudioPlayer::GetDsdModes() const noexcept {
    return audio_config_.dsd_mode;
}

void AudioPlayer::CloseDevice(bool wait_for_stop_stream, bool quit) {
    playback_state_.is_playing = false;
    playback_state_.is_paused = false;
    pause_cond_.notify_all();
    read_finish_and_wait_seek_signal_cond_.notify_all();

    if (stream_task_.valid()) {
        XAMP_LOG_D(logger_, "Try to stop stream thread.");
#ifdef XAMP_OS_WIN
        // MSVC 2019 is wait for std::packaged_task return timeout, while others such clang can't.
        //if (stream_task_.wait_for(kWaitForStreamStopTime) == std::future_status::timeout) {
        //    throw StopStreamTimeoutException();
        //}
        stream_task_.get();
#else
        stream_task_.get();
#endif
        stream_task_ = Task<void>();
        XAMP_LOG_D(logger_, "Stream thread was finished.");
    }

    playback_state_.stream_offset_time = 0;

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

    PlayerAction dummy;
    while (action_queue_.try_dequeue(dummy)) {
    }
}

void AudioPlayer::ResizeReadBuffer(uint32_t allocate_size) {
    if (read_buffer_.GetSize() == 0
        || read_buffer_.GetSize() != allocate_size) {
        XAMP_LOG_D(logger_, "Allocate read buffer : {}.", 
            String::FormatBytes(allocate_size));
        read_buffer_ = MakeBuffer<std::byte>(allocate_size);
    }
}

void AudioPlayer::ResizeFIFO(uint32_t fifo_size) {
    if (fifo_.GetSize() == 0 || fifo_.GetSize() < fifo_size) {
        XAMP_LOG_D(logger_, "Allocate fifo buffer : {}.",
            String::FormatBytes(fifo_size));
        fifo_.Resize(fifo_size);
    }
	fifo_.Clear();
}

void AudioPlayer::CreateBuffer() {
    uint32_t allocate_size = 0;
    uint32_t fifo_size = 0;

	auto align_page_size = [](uint32_t size) {
		return AlignUp(size, GetPageSize());
		};

    auto get_buffer_sample = [](auto* device, auto ratio) {
        return static_cast<uint32_t>(AlignUp(
            device->GetBufferSize() * ratio, GetPageSize()));
        };

    if (audio_config_.dsd_mode == DsdModes::DSD_MODE_NATIVE) {
        // DSD native mode
        num_read_buffer_size_ = align_page_size(
            output_format_.GetSampleRate() / 8);
        num_write_buffer_size_ = device_->GetBufferSize() * kMaxBufferSecs;
    }
    else {
        auto max_ratio = std::max(
            output_format_.GetAvgBytesPerSec() / input_format_.GetAvgBytesPerSec(), 1U);
        num_write_buffer_size_ = get_buffer_sample(device_.get(),
            max_ratio * sizeof(float));
        num_read_buffer_size_ = std::max(get_buffer_sample(
            device_.get(), kMaxReadRatio), kMinReadBufferSize);
    }

    if (audio_config_.dsd_mode == DsdModes::DSD_MODE_NATIVE) {
        allocate_size = num_read_buffer_size_;
        fifo_size = output_format_.GetAvgBytesPerSec() * kMaxBufferSecs;
    }
    else {
        if (!enable_file_cache_) {
            fifo_size = kMaxBufferSecs
        	* output_format_.GetAvgBytesPerSec()
        	* GetBufferCount(output_format_.GetSampleRate());
        }
        else {
            fifo_size = kMaxPreAllocateBufferSize;
        }
        fifo_size = align_page_size(fifo_size);

        allocate_size = std::min(kMaxPreAllocateBufferSize,
            num_write_buffer_size_ 
            * stream_->GetSampleSize() 
            * kTotalBufferStreamCount);
        allocate_size = align_page_size(allocate_size);
    }

    ResizeReadBuffer(allocate_size);
    ResizeFIFO(fifo_size);

    XAMP_LOG_DEBUG(
        "Device output buffer:{} num_write_buffer_size:{} num_read_sample:{} fifo buffer:{}.",
        String::FormatBytes(device_->GetBufferSize()),
        String::FormatBytes(num_write_buffer_size_),
        String::FormatBytes(num_read_buffer_size_),
        String::FormatBytes(fifo_size));
}

void AudioPlayer::SetDeviceFormat() {
    if (audio_config_.target_sample_rate != 0 
        && IsPcmAudio(audio_config_.dsd_mode)) {
        if (output_format_.GetSampleRate() != audio_config_.target_sample_rate) {
            device_id_.clear();
        }
        output_format_ = input_format_;
        output_format_.SetSampleRate(audio_config_.target_sample_rate);
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
    playback_state_.is_playing = false;
    if (const auto adapter = state_adapter_.lock()) {
        adapter->OnError(e);
    }
}

void AudioPlayer::OnDeviceStateChange(DeviceState state, const std::string & device_id) {    
    if (const auto state_adapter = state_adapter_.lock()) {
        switch (state) {
        case DeviceState::DEVICE_STATE_ADDED:
            XAMP_LOG_D(logger_, "Device added device id:{}.", device_id);
            state_adapter->OnDeviceChanged(
                DeviceState::DEVICE_STATE_ADDED,
                device_id);
            break;
        case DeviceState::DEVICE_STATE_REMOVED:
            XAMP_LOG_D(logger_, "Device removed device id:{}.", device_id);
            if (device_id == device_id_) {
                // TODO: In many system has more ASIO device.
                if (IsAsioDevice(device_type_->GetTypeId())) {
                    ResetAsioDriver();
                }
                
                state_adapter->OnDeviceChanged(
                    DeviceState::DEVICE_STATE_REMOVED,
                    device_id);
                if (device_ != nullptr) {
                    device_->AbortStream();
                    XAMP_LOG_D(logger_, "Device abort stream id:{}.", device_id);
                }
            }
            break;
        case DeviceState::DEVICE_STATE_DEFAULT_DEVICE_CHANGE:
            XAMP_LOG_D(logger_, "Default device device id:{}.", device_id);
            state_adapter->OnDeviceChanged(
                DeviceState::DEVICE_STATE_DEFAULT_DEVICE_CHANGE,
                device_id);
            break;
        }
    }
}

void AudioPlayer::OpenDevice(double stream_time) {
#if defined(XAMP_OS_WIN)
    if (auto* dsd_output = AsDsdDevice(device_)) {
        if (audio_config_.dsd_mode == DsdModes::DSD_MODE_AUTO
            || audio_config_.dsd_mode == DsdModes::DSD_MODE_PCM
            || audio_config_.dsd_mode == DsdModes::DSD_MODE_DOP) {
            if (const auto* const dsd_stream = AsDsdStream(stream_)) {
                if (audio_config_.dsd_mode == DsdModes::DSD_MODE_NATIVE) {
                    dsd_output->SetIoFormat(DsdIoFormat::IO_FORMAT_DSD);
                }
                else {
                    if (audio_config_.dsd_mode == DsdModes::DSD_MODE_DOP) {
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

void AudioPlayer::BufferStream(double stream_time, 
    const std::optional<double>& offset,
    const std::optional<double>& duration) {
    XAMP_LOG_D(logger_, "Buffing samples : {:.2f}ms", stream_time);

	if (dsp_manager_->Contains(XAMP_UUID_OF(R8brainSampleRateConverter))) {
        SetReadSampleSize(kR8brainBufferSize);
	}

    if (offset.has_value()) {
        playback_state_.stream_offset_time = offset.value();
    }

    if (duration.has_value()) {
        playback_state_.stream_duration = duration.value();
    }

    fifo_.Clear();
    stream_->SeekAsSeconds(playback_state_.stream_offset_time + stream_time);
    audio_config_.sample_size = stream_->GetSampleSize();
    BufferSamples(stream_, GetBufferCount(output_format_.GetSampleRate()));
}

void AudioPlayer::EnableFadeOut(bool enable) {
    enable_fadeout_ = enable;
}

void AudioPlayer::UpdatePlayerStreamTime(uint32_t stream_time_sec_unit) noexcept {
    playback_state_.stream_time_sec_unit.exchange(stream_time_sec_unit);
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

        if (p->playback_state_.is_paused) {
            return;
        }
        if (!p->playback_state_.is_playing) {
            return;
        }
        if (p->playback_state_.is_seeking) {
            return;
        }
        const auto stream_time_sec_unit =
            p->playback_state_.stream_time_sec_unit.load();
        if (p->playback_state_.is_playing) {
            if (stream_time_sec_unit == kStopStreamTime) {
                p->SetState(PlayerState::PLAYER_STATE_STOPPED);
                p->playback_state_.is_playing = false;
            }
        }
        });
}

void AudioPlayer::Seek(double stream_time) {
    if (!device_) {
        return;
    }

    if (device_->IsStreamOpen()) {
        read_finish_and_wait_seek_signal_cond_.notify_one();
		playback_state_.is_seeking = true;
        action_queue_.try_enqueue(PlayerAction{
            PlayerActionId::PLAYER_SEEK,
            SeekAction{ stream_time }
            });
    }
}

void AudioPlayer::DoSeek(double stream_time) {
	if (playback_state_.state != PlayerState::PLAYER_STATE_PAUSED) {
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
    auto seek_time = static_cast<uint32_t>(stream_time * 1000.0);
    if (seek_time >playback_state_.stream_time_sec_unit) {
        seek_time = static_cast<uint32_t>(Round(stream_time, 2) * 1000.0);
    }
    UpdatePlayerStreamTime(seek_time);
    fifo_.Clear();
    BufferStream(stream_time);
    Resume();
}

void AudioPlayer::BufferSamples(const ScopedPtr<FileStream>& stream,
    int32_t buffer_count) {
    auto* const sample_buffer = read_buffer_.Get();

    for (auto i = 0; i < buffer_count && stream_->IsActive(); ++i) {
        XAMP_LOG_D(logger_, "Buffering {} ...", i);

        while (true) {
            const auto num_samples = stream->GetSamples(sample_buffer, 
                num_read_buffer_size_);
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

const ScopedPtr<IAudioDeviceManager>& AudioPlayer::GetAudioDeviceManager() {
    return device_manager_;
}

ScopedPtr<IDSPManager>& AudioPlayer::GetDspManager() {
    return dsp_manager_;
}

void AudioPlayer::SetReadSampleSize(uint32_t num_samples) {
    num_read_buffer_size_ = num_samples;
    XAMP_LOG_D(logger_,
        "Output buffer:{} device format: {} num_read_sample: {} fifo buffer: {}.",
        device_->GetBufferSize(),
        output_format_,
        String::FormatBytes(num_read_buffer_size_),
        String::FormatBytes(fifo_.GetSize()));
}

void AudioPlayer::WaitForReadFinishAndSeekSignal(
    std::unique_lock<FastMutex>& stopped_lock) {
    if (read_finish_and_wait_seek_signal_cond_.wait_for(stopped_lock,
        kWaitForSignalWhenReadFinish) != std::cv_status::timeout) {
        XAMP_LOG_T(logger_, "Stream is read done!, Weak up for seek signal.");
    }
}

bool AudioPlayer::ShouldKeepReading() const noexcept {
    return playback_state_.is_playing && stream_->IsActive();
}

void AudioPlayer::ReadSampleLoop(std::byte* buffer,
    uint32_t buffer_size, 
    std::unique_lock<FastMutex>& stopped_lock) {
    if (!stream_->IsActive()) {
        if (playback_state_.is_playing) {
            WaitForReadFinishAndSeekSignal(stopped_lock);
        }
        return;
    }    

	auto* bass_stream = dynamic_cast<BassFileStream*>(stream_.get());

    while (ShouldKeepReading()) {
        const auto num_samples = stream_->GetSamples(buffer, buffer_size);

        if (num_samples > 0) {
            auto* samples = reinterpret_cast<float*>(buffer);
            
            if (dsp_manager_->ProcessDSP(samples, num_samples, fifo_)) {
                continue;
            }
        }

        if (num_samples == 0) {
            if (bass_stream != nullptr && !bass_stream->EndOfStream()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                XAMP_LOG_I(logger_, "Player waiting data ...");
                continue;
            }
        }

        if (!enable_file_cache_ || !IsAvailableWrite()) {
            break;
        }        
    }
}

bool AudioPlayer::IsAvailableWrite() const noexcept {
    const auto num_write_buffer_size =
        num_write_buffer_size_ * kMaxWriteRatio;
    return fifo_.GetAvailableWrite() >= num_write_buffer_size;
}

void AudioPlayer::Play() {
    XAMP_LOG_W(logger_, "Player is playing!");

    if (!device_) {
        return;
    }

    if (const auto state = state_adapter_.lock()) {
        state->OutputFormatChanged(output_format_, device_->GetBufferSize());
    }

    is_fade_out_ = false;
    playback_state_.is_playing = true;
    if (device_->IsStreamOpen()) {
        if (!device_->IsStreamRunning()) {
            XAMP_LOG_D(logger_, "Play volume:{} muted:{}.",
                audio_config_.volume, is_muted_);
            device_->StartStream();
            SetState(PlayerState::PLAYER_STATE_RUNNING);
        }
    }

    if (stream_task_.valid()) {
        XAMP_LOG_W(logger_, "Stream task is valid!");
        return;
    }

    XAMP_LOG_W(logger_, "Stream task is spawning!");

    stream_task_ = Executor::Spawn(player_thread_pool_.get(),
        [player = shared_from_this()](const auto& stop_token) {
        XAMP_LOG_W(player->logger_, "Stream task is spawn done!");

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
            while (p->playback_state_.is_playing && !stop_token.stop_requested()) {
                // Wait for pause signal.
                while (p->playback_state_.is_paused) {
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
    const auto adapter = state_adapter_.lock();
    if (!adapter) {
        return;
    }

    auto stream_time_sec_unit = playback_state_.stream_time_sec_unit.load();
    adapter->OnSampleTime(stream_time_sec_unit / 1000.0);

    if (!IsPcmAudio(audio_config_.dsd_mode)) {
        return;
    }

    Stopwatch watch;    
    watch.Reset();    
    
    adapter->OnSamplesChanged(static_cast<const float*>(samples), num_buffer_frames);
    auto elapsed = watch.Elapsed<std::chrono::milliseconds>();
    if (elapsed >= kMinimalCopySamplesTime) {
        XAMP_LOG_W(logger_, "CopySamples too slow ({} ms)!", elapsed.count());
    }
}

DataCallbackResult AudioPlayer::OnGetSamples(void* samples,
    size_t num_buffer_frames, 
    size_t & num_filled_frames, 
    double stream_time, 
    double /*sample_time*/) noexcept {
    // sample_time: 指的是設備撥放時間, 會被stop時候重置為0.
    // stream time: samples大小累計從開始撥放到結束的時間.
    const auto num_samples = num_buffer_frames * output_format_.GetChannels();
    const auto sample_size = num_samples * audio_config_.sample_size;

    if (is_fade_out_) {
        FadeOut();
        is_fade_out_ = false;
    }

    if (stream_time >= playback_state_.stream_duration) {
        UpdatePlayerStreamTime(kStopStreamTime);
        return DataCallbackResult::STOP;
    }

    size_t num_filled_bytes = 0;
    if (fifo_.TryRead(static_cast<std::byte*>(samples),
        sample_size, 
        num_filled_bytes)) {
        num_filled_frames = num_filled_bytes
    	/ audio_config_.sample_size
    	/ output_format_.GetChannels();
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
    if (stream_time >= playback_state_.stream_duration) {
        UpdatePlayerStreamTime(kStopStreamTime);
        return DataCallbackResult::STOP;
    }

    num_filled_frames = num_buffer_frames;
    if (playback_state_.is_seeking) {
        UpdatePlayerStreamTime(static_cast<int32_t>(stream_time * 1000));
    }
    return DataCallbackResult::CONTINUE;
}

void AudioPlayer::PrepareToPlay(ByteFormat byte_format,
    uint32_t device_sample_rate) {
    if (device_sample_rate != 0) {
        audio_config_.target_sample_rate = device_sample_rate;
    }

    SetDeviceFormat();

	if (byte_format != ByteFormat::INVALID_FORMAT) {
        output_format_.SetByteFormat(byte_format);
    }

    CreateDevice(device_info_.value().device_type_id,
        device_info_.value().device_id,
        false);
    OpenDevice(0);
    CreateBuffer();

    config_.AddOrReplace(DspConfig::kInputFormat,
        std::any(input_format_));
    config_.AddOrReplace(DspConfig::kOutputFormat,
        std::any(output_format_));
    config_.AddOrReplace(DspConfig::kDsdMode, 
        std::any(audio_config_.dsd_mode));
    config_.AddOrReplace(DspConfig::kSampleSize, 
        std::any(stream_->GetSampleSize()));

    dsp_manager_->Initialize(config_);
	sample_end_time_ = stream_->GetDurationAsSeconds();
    XAMP_LOG_D(logger_, "Stream end time: {:.2f} sec.", sample_end_time_);    
}

AnyMap& AudioPlayer::GetDspConfig() {
    return config_;
}

void AudioPlayer::SetDelayCallback(std::function<void(uint32_t)>&& delay_callback) {
    delay_callback_ = std::forward<std::function<void(uint32_t)>>(delay_callback);
}

void AudioPlayer::SeFileCacheMode(bool enable) {
    enable_file_cache_ = enable;
}

XAMP_AUDIO_PLAYER_NAMESPACE_END
