#include <base/str_utilts.h>
#include <base/platform_thread.h>
#include <base/logger.h>
#include <base/stl.h>
#include <base/threadpool.h>

#include <output_device/audiodevicemanager.h>
#include <output_device/asiodevicetype.h>
#include <output_device/dsddevice.h>

#include <stream/bassfilestream.h>
#include <stream/avfilestream.h>
#include <stream/bassequalizer.h>

#include <player/soxresampler.h>
#include <player/resampler.h>
#include <player/nullresampler.h>
#include <player/chromaprint.h>
#include <player/audio_util.h>
#include <player/audio_player.h>

namespace xamp::player {

inline constexpr int32_t kMinEQSampleRate = 44100;
inline constexpr int32_t kBufferStreamCount = 5;
inline constexpr int32_t kTotalBufferStreamCount = 10;
inline constexpr int32_t kPreallocateBufferSize = 16 * 1024 * 1024;
inline constexpr int32_t kMaxWriteRatio = 20;
inline constexpr int32_t kMaxReadRatio = 30;

inline constexpr std::chrono::milliseconds kUpdateSampleInterval(30);
inline constexpr std::chrono::milliseconds kReadSampleWaitTime(30);
inline constexpr std::chrono::seconds kWaitForStreamStopTime(10);

static void LogTime(std::string msg, std::chrono::microseconds time) {
    auto c = time.count();
    XAMP_LOG_DEBUG(msg + ": {}.{:03}'{:03}sec",
        (c % 1'000'000'000) / 1'000'000, (c % 1'000'000) / 1'000, c % 1'000);
}

AudioPlayer::AudioPlayer()
    : AudioPlayer(std::weak_ptr<PlaybackStateAdapter>()) {
}

AudioPlayer::AudioPlayer(std::weak_ptr<PlaybackStateAdapter> adapter)
    : is_muted_(false)
    , enable_resample_(false)
    , dsd_mode_(DsdModes::DSD_MODE_PCM)
    , state_(PlayerState::PLAYER_STATE_STOPPED)
    , sample_size_(0)
    , target_samplerate_(0)
    , volume_(0)
    , num_buffer_samples_(0)
    , num_read_sample_(0)
    , read_sample_size_(0)
    , is_playing_(false)
    , is_paused_(false)
    , enable_gapless_play_(false)
    , sample_end_time_(0)
    , stream_duration_(0)
    , state_adapter_(adapter)
    , buffer_(kPreallocateBufferSize)
    , msg_queue_(kMsgQueueSize) {
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
    resampler_.reset();
    equalizer_.reset();
#ifdef ENABLE_ASIO
    AudioDeviceManager::RemoveASIODriver();
#endif
    BassFileStream::FreeBassLib();
}

void AudioPlayer::UpdateSlice(float const *samples, int32_t sample_size, double stream_time) noexcept {
    std::atomic_exchange_explicit(&slice_,
        AudioSlice{ samples, sample_size, stream_time },
        std::memory_order_relaxed);
}

void AudioPlayer::Initial() {
    AudioDeviceManager::PreventSleep(true);

    ThreadPool::GetInstance();

    (void)AudioDeviceManager::GetInstance();
    XAMP_LOG_DEBUG("AudioDeviceManager init success.");

    BassFileStream::LoadBassLib();
    XAMP_LOG_DEBUG("Load BASS dll success.");

    SoxrResampler::LoadSoxrLib();
    XAMP_LOG_DEBUG("Load Soxr dll success.");
	
    try {
        Chromaprint::LoadChromaprintLib();
        XAMP_LOG_DEBUG("Load Chromaprint dll success.");
    }
    catch (...) {
    	// Ignore exception.
    }   

    try {
        BassEqualizer::LoadBassFxLib();
        XAMP_LOG_DEBUG("Load BASS Fx dll success.");
    }
    catch (...) {
        // Ignore exception.
    }     
}

void AudioPlayer::Open(std::wstring const & file_path, 
    std::wstring const & file_ext,
    DeviceInfo const & device_info, 
    bool use_native_dsd) {
    Startup();
    CloseDevice(true);
    OpenStream(file_path, file_ext, device_info, use_native_dsd);
    device_info_ = device_info;
}

void AudioPlayer::SetResampler(uint32_t samplerate, AlignPtr<Resampler>&& resampler) {
    target_samplerate_ = samplerate;
    resampler_ = std::move(resampler);
    EnableResampler(true);
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
        }
        if (auto result = AudioDeviceManager::GetInstance().Create(device_type_id)) {            
            device_type_ = std::move(result.value());
            // TODO: remove ScanNewDevice ?
            device_type_->ScanNewDevice();
            device_ = device_type_->MakeDevice(device_id);
            device_type_id_ = device_type_id;
            device_id_ = device_id;
            XAMP_LOG_DEBUG("Create device: {}", device_type_->GetDescription());
        }
        else {
            throw DeviceNotFoundException();
        }
    }
    device_->SetAudioCallback(this);
}

bool AudioPlayer::IsDSDFile() const {
    if (!stream_) {
        return false;
    }
    if (auto dsd_stream = dynamic_cast<DsdStream const*>(stream_.get())) {
        return dsd_stream->IsDsdFile();
    }
    return false;
}

bool AudioPlayer::IsDsdStream() const noexcept {
    if (!stream_) {
        return false;
    }
    return dynamic_cast<DsdStream*>(stream_.get()) != nullptr;
}

void AudioPlayer::OpenStream(std::wstring const & file_path, std::wstring const & file_ext, DeviceInfo const & device_info, bool use_native_dsd) {
    stream_ = MakeFileStream(file_ext, std::move(stream_));
    dsd_mode_ = SetStreamDsdMode(stream_, device_info, use_native_dsd);
    XAMP_LOG_DEBUG("Use stream type: {}.", stream_->GetDescription());
    stream_->OpenFile(file_path);
    stream_duration_ = stream_->GetDuration();
    if (auto stream = AsDsdStream(stream_)) {
        dsd_mode_ = stream->GetDsdMode();
    }    
}

void AudioPlayer::SetState(const PlayerState play_state) {
    if (auto adapter = state_adapter_.lock()) {
        adapter->OnStateChanged(play_state);
    }
    state_ = play_state;
    XAMP_LOG_DEBUG("Set state: {}.", EnumToString(state_));
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
            XAMP_LOG_DEBUG("Volume:{} IsMuted:{}.", volume_, is_muted_);
            device_->StartStream();
            SetState(PlayerState::PLAYER_STATE_RUNNING);
        }
    }
}

void AudioPlayer::Pause() {
    if (!device_) {
        return;
    }

    if (!is_paused_) {
        XAMP_LOG_DEBUG("Player pasue.");
        if (device_->IsStreamOpen()) {
            is_paused_ = true;
            device_->StopStream();
            SetState(PlayerState::PLAYER_STATE_PAUSED);
        }
    }
}

void AudioPlayer::Resume() {
    if (!device_) {
        return;
    }

    XAMP_LOG_DEBUG("Player resume.");
    if (device_->IsStreamOpen()) {
        SetState(PlayerState::PLAYER_STATE_RESUME);
        is_paused_ = false;
        pause_cond_.notify_all();
        stopped_cond_.notify_all();
        device_->StartStream();
        SetState(PlayerState::PLAYER_STATE_RUNNING);
    }
}

void AudioPlayer::Stop(bool signal_to_stop, bool shutdown_device, bool wait_for_stop_stream) {
    if (!device_) {
        return;
    }

    XAMP_LOG_DEBUG("Player stop.");
    if (device_->IsStreamOpen()) {
        CloseDevice(wait_for_stop_stream);
        UpdateSlice();
        if (signal_to_stop) {
            SetState(PlayerState::PLAYER_STATE_STOPPED);                        
        }
    }    

    buffer_.Clear();
    if (shutdown_device) {
        device_.reset();
        device_id_.clear();
    }
    stream_.reset();
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
    std::lock_guard guard{ stream_read_mutex_ };

    if (!stream_) {
        return std::nullopt;
    }

    if (auto* const dsd_stream = AsDsdStream(stream_)) {
        if (dsd_stream->IsDsdFile()) {
            return dsd_stream->GetDsdSpeed();
        }
    }
    return std::nullopt;
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

AudioFormat AudioPlayer::GetFileFormat() const noexcept {
    std::lock_guard guard{ stream_read_mutex_ };
    return stream_->GetFormat();
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
            XAMP_LOG_DEBUG("Stop output device");
            try {
                device_->StopStream(wait_for_stop_stream);
                device_->CloseStream();
            } catch (const Exception &e) {
                XAMP_LOG_DEBUG("Close device failure. {}", e.what());
            }
        }
    }

    if (stream_task_.valid()) {
        XAMP_LOG_DEBUG("Try to stop stream thread.");        
#ifdef XAMP_OS_WIN
        // MSVC 2019 is wait for std::packaged_task return timeout, while others such clang can't.
        if (stream_task_.wait_for(kWaitForStreamStopTime) == std::future_status::timeout) {
            throw StopStreamTimeoutException();
        }
#else
        Stopwatch sw;
        stream_task_.get();
        LogTime("Thread switch time", sw.Elapsed());
#endif
        XAMP_LOG_DEBUG("Stream thread was finished.");
    }
    buffer_.Clear();
#ifdef DEBUG
    LogTime("Device max process time", min_process_time_);
    LogTime("Device min process time", min_process_time_);
#endif
}

void AudioPlayer::CreateBuffer() {
    UpdateSlice();

    uint32_t require_read_sample = 0;

    if (AudioDeviceManager::GetInstance().IsSupportASIO()) {
        if (dsd_mode_ == DsdModes::DSD_MODE_NATIVE
            || AudioDeviceManager::GetInstance().IsASIODevice(device_type_id_)) {
            require_read_sample = kMaxSamplerate;
        }
        else {
            require_read_sample = device_->GetBufferSize() * kMaxReadRatio;
        }
    }
    else {
        require_read_sample = device_->GetBufferSize() * kMaxReadRatio;
    }

    if (require_read_sample != num_read_sample_) {
        auto allocate_size = require_read_sample * stream_->GetSampleSize() * kBufferStreamCount;
        num_buffer_samples_ = allocate_size * kTotalBufferStreamCount;
        num_read_sample_ = require_read_sample;
        XAMP_LOG_DEBUG("Allocate interal buffer : {}.", FormatBytes(allocate_size));
        sample_buffer_ = AlignedBuffer<int8_t>(allocate_size);
        sample_buffer_lock_.Lock(sample_buffer_.Get(), sample_buffer_.GetByteSize());
        read_sample_size_ = allocate_size;
    }

    if (!enable_resample_
        || dsd_mode_ == DsdModes::DSD_MODE_NATIVE
        || dsd_mode_ == DsdModes::DSD_MODE_DOP) {
        resampler_ = MakeAlign<Resampler, NullResampler>(dsd_mode_, stream_->GetSampleSize());
    }

    if (!enable_resample_) {
        if (buffer_.GetSize() == 0 || buffer_.GetSize() < num_buffer_samples_) {
            XAMP_LOG_DEBUG("Buffer too small reallocate.");
            buffer_.Resize(num_buffer_samples_);
            XAMP_LOG_DEBUG("Allocate player buffer : {}.", FormatBytes(num_buffer_samples_));
        }
    }
    else {
        assert(target_samplerate_ > 0);
        resampler_->Start(input_format_.GetSampleRate(),
                          input_format_.GetChannels(),
                          target_samplerate_,
                          num_read_sample_ / input_format_.GetChannels());
    }

    XAMP_LOG_DEBUG("Output device format: {} num_read_sample: {} resampler: {} buffer: {}.",
                   output_format_,
                   num_read_sample_,
                   resampler_->GetDescription(),
                   FormatBytes(buffer_.GetSize()));
}

void AudioPlayer::EnableResampler(bool enable) {
    enable_resample_ = enable;
}

bool AudioPlayer::IsEnableResampler() const {
    return enable_resample_;
}

void AudioPlayer::SetDeviceFormat() {
    input_format_ = stream_->GetFormat();

    if (enable_resample_ && dsd_mode_ == DsdModes::DSD_MODE_PCM) {
        if (output_format_.GetSampleRate() != target_samplerate_) {
            device_id_.clear();
        }
        output_format_ = input_format_;
        output_format_.SetSampleRate(target_samplerate_);
    }
    else {
        if (input_format_.GetSampleRate() != output_format_.GetSampleRate()) {
            device_id_.clear();
        }
        output_format_ = input_format_;
    }
}

void AudioPlayer::OnVolumeChange(float vol) noexcept {
    if (auto adapter = state_adapter_.lock()) {
        adapter->OnVolumeChanged(vol);
        XAMP_LOG_DEBUG("Volum change: {}.", vol);
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
            XAMP_LOG_DEBUG("Device added device id:{}.", device_id);
            state_adapter->OnDeviceChanged(DeviceState::DEVICE_STATE_ADDED);
            break;
        case DeviceState::DEVICE_STATE_REMOVED:
            XAMP_LOG_DEBUG("Device removed device id:{}.", device_id);
            if (device_id == device_id_) {
                state_adapter->OnDeviceChanged(DeviceState::DEVICE_STATE_REMOVED);
                if (device_ != nullptr) {
                    device_->AbortStream();
                    XAMP_LOG_DEBUG("Device abort stream id:{}.", device_id);
                }
            }
            break;
        case DeviceState::DEVICE_STATE_DEFAULT_DEVICE_CHANGE:
            XAMP_LOG_DEBUG("Default device device id:{}.", device_id);
            state_adapter->OnDeviceChanged(DeviceState::DEVICE_STATE_DEFAULT_DEVICE_CHANGE);
            break;
        }
    }
}

void AudioPlayer::OpenDevice(double stream_time) {
#ifdef ENABLE_ASIO
    if (auto dsd_output = AsDsdDevice(device_)) {
        if (const auto dsd_stream = AsDsdStream(stream_)) {
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

    if (equalizer_ != nullptr) {
        if (output_format_.GetSampleRate() == kMinEQSampleRate) {
            equalizer_ = MakeAlign<Equalizer, BassEqualizer>();
            equalizer_->Start(output_format_.GetChannels(), output_format_.GetSampleRate());
            equalizer_->SetEQ(eqsettings_);
        }
        else {
            equalizer_.reset();
        }
    }
}

void AudioPlayer::EnableEQ(bool enable) {
    if (enable) {
        equalizer_ = MakeAlign<Equalizer, BassEqualizer>();
    }
    else {
        equalizer_.reset();
    }
}

void AudioPlayer::SetEQ(std::array<EQSettings, kMaxBand> const &bands) {
    eqsettings_ = bands;
}

void AudioPlayer::SetEQ(uint32_t band, float gain, float Q) {    
    if (band >= eqsettings_.size()) {
        return;
    }
    eqsettings_[band].gain = gain;
    eqsettings_[band].Q = Q;
}

void AudioPlayer::SetLoop(double start_time, double end_time) {
    Seek(start_time);
    sample_end_time_ = end_time - start_time;
}

void AudioPlayer::Seek(double stream_time) {
    if (!device_) {
        return;
    }
    if (device_->IsStreamOpen()) {
        Pause();
        gapless_play_state_ = GaplessPlayState::INIT;

        try {
            stream_->Seek(stream_time);
        }
        catch (std::exception const & e) {
            XAMP_LOG_DEBUG(e.what());
            Resume();
            return;
        }
        device_->SetStreamTime(stream_time);
        sample_end_time_ = stream_->GetDuration() - stream_time;
        XAMP_LOG_DEBUG("Player duration:{} seeking:{} sec, end time:{} sec.",
                       stream_->GetDuration(),
                       stream_time,
                       sample_end_time_);
        UpdateSlice(nullptr, 0, stream_time);
        buffer_.Clear();
        BufferStream(stream_time);
        Resume();
    }
}

void AudioPlayer::BufferStream(double stream_time) {
    buffer_.Clear();

    if (stream_time > 0.0) {
        stream_->Seek(stream_time);
    }

    if (enable_resample_) {
        resampler_->Flush();
    }

    sample_size_ = stream_->GetSampleSize();
    BufferSamples(stream_, resampler_, kBufferStreamCount);
}

void AudioPlayer::BufferSamples(AlignPtr<FileStream> &stream, AlignPtr<Resampler>& resampler, int32_t buffer_count) {
    auto* const sample_buffer = sample_buffer_.Get();
    
    for (auto i = 0; i < buffer_count; ++i) {
        while (true) {
            const auto num_samples = stream->GetSamples(sample_buffer, num_read_sample_);
            if (num_samples == 0) {
                return;
            }

            const auto samples = reinterpret_cast<const float*>(sample_buffer);

            auto use_resampler = true;
            if (equalizer_ != nullptr) {
                if (dsd_mode_ == DsdModes::DSD_MODE_PCM) {
                    equalizer_->Process(samples, num_samples, buffer_);
                    use_resampler = false;
                }
            }

            if (use_resampler) {
                if (!resampler->Process(samples, num_samples, buffer_)) {
                    continue;
                }
            }
            break;
        }
    }
}

void AudioPlayer::Startup() {
    if (timer_.IsStarted()) {
        return;
    }

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

void AudioPlayer::OnGaplessPlayState(std::unique_lock<std::mutex>& lock, AlignPtr<Resampler> &resampler) {
    auto adapter = state_adapter_.lock();
    if (!adapter) {
        return;
    }

    if (adapter->GetPlayQueueSize() == 0 || !enable_gapless_play_) {
        stopped_cond_.wait_for(lock, kReadSampleWaitTime);
        return;
    }

    std::lock_guard guard{ stream_read_mutex_ };

    auto& stream = adapter->PlayQueueFont();

    if (msg_queue_.size() > 0) {
        // 處理播放完畢後替換掉Stream.
        auto msg_id = *msg_queue_.Front();

        switch (msg_id) {
        case GaplessPlayMsgID::SWITCH:
            XAMP_LOG_DEBUG("Receive SWITCH");
            adapter->OnGaplessPlayback();
            stream_->Close();
            stream_ = std::move(stream);
            dsd_mode_ = SetStreamDsdMode(stream_, device_info_);
            resampler_ = std::move(resampler);
            adapter->PopPlayQueue();
            stream_duration_ = stream_->GetDuration();
            msg_queue_.Pop();
            gapless_play_state_ = GaplessPlayState::INIT;
            break;
        }
        return;
    }

    if (gapless_play_state_ == GaplessPlayState::INIT) {
        XAMP_LOG_DEBUG("Gapless play initial!");
        // 有可能使用者進行Seek的動作需要重置位置.
        stream->Seek(0);
        resampler = resampler_->Clone();
        auto input_format = stream->GetFormat();
        resampler->Start(input_format.GetSampleRate(),
            input_format.GetChannels(),
            target_samplerate_,
            num_read_sample_ / input_format_.GetChannels());
        gapless_play_state_ = GaplessPlayState::BUFFING;
    }

    if (gapless_play_state_ == GaplessPlayState::BUFFING) {
        // 等待上一首播放結束並繼續緩衝資料.
        BufferSamples(stream, resampler);        
    }
}

void AudioPlayer::ReadSampleLoop(int8_t *sample_buffer, uint32_t max_read_sample, std::unique_lock<std::mutex>& lock, AlignPtr<Resampler>& resampler) {
    while (is_playing_) {
        const auto num_samples = stream_->GetSamples(sample_buffer, max_read_sample);

        if (num_samples > 0) {
	        const auto samples = reinterpret_cast<const float*>(sample_buffer_.Get());

            auto use_resampler = true;
            if (equalizer_ != nullptr) {
                if (dsd_mode_ == DsdModes::DSD_MODE_PCM) {
                    equalizer_->Process(samples, num_samples, buffer_);
                    use_resampler = false;
                }
            } 

            if (use_resampler) {
                if (!resampler_->Process(samples, num_samples, buffer_)) {
                    continue;
                }
            }
            // TODO: 如果在這裡做資料轉換(ex:float->int32_t),
            // 可能需要處理GetSamples不滿足轉換上長度的需求.
        }
        else {            
            OnGaplessPlayState(lock, resampler);
        }
        break;
    }
}

void AudioPlayer::EnableGaplessPlay(bool enable) {
    std::lock_guard guard{ stream_read_mutex_ };
    enable_gapless_play_ = enable;
    if (enable == false) {
        if (auto adapter = state_adapter_.lock()) {
            adapter->ClearPlayQueue();
        }
    }    
}

bool AudioPlayer::IsGaplessPlay() const {
    return IsPlaying() && enable_gapless_play_;
}

void AudioPlayer::ClearPlayQueue() {
    std::lock_guard guard{ stream_read_mutex_ };
    if (auto adapter = state_adapter_.lock()) {
        adapter->ClearPlayQueue();
    }
}

int32_t AudioPlayer::OnGetSamples(void* samples, uint32_t num_buffer_frames, double stream_time, double sample_time) noexcept {
    const auto num_samples = num_buffer_frames * output_format_.GetChannels();
    const auto sample_size = num_samples * sample_size_;
    
#ifdef _DEBUG
    auto elapsed = sw_.Elapsed();
    max_process_time_ = std::max(elapsed, max_process_time_);
    min_process_time_ = std::min(elapsed, min_process_time_);
#endif

//  TODO: Gapless play不支援以後需要調整.
//#ifdef XAMP_OS_WIN
//    if (sample_time > sample_end_time_) {
//        std::memset(static_cast<int8_t*>(samples), 0, sample_size);
//    }
//    else
//#endif
    {
        if (stream_time >= stream_duration_ && enable_gapless_play_) {
            device_->SetStreamTime(0);
            msg_queue_.TryPush(GaplessPlayMsgID::SWITCH);
        }

        if (XAMP_LIKELY(buffer_.TryRead(static_cast<int8_t*>(samples), sample_size))) {
            UpdateSlice(static_cast<const float*>(samples), static_cast<int32_t>(num_samples), stream_time);
#ifdef _DEBUG
            sw_.Reset();
#endif
            return 0;
        }

        if (sample_time <= sample_end_time_) {
            std::memset(static_cast<int8_t*>(samples), 0, sample_size);
            return 0;
        }
    }

    UpdateSlice(nullptr, -1, stream_time);
    return 1;
}

void AudioPlayer::StartPlay(double start_time, double end_time) {
    SetDeviceFormat();
    CreateDevice(device_info_.device_type_id, device_info_.device_id, false);
    OpenDevice(start_time);
    CreateBuffer();
    BufferStream(start_time);

#ifdef _DEBUG
    sw_.Reset();
    min_process_time_ = std::chrono::seconds(1);
    max_process_time_ = std::chrono::microseconds(0);
#endif

    if (end_time > 0.0) {
        sample_end_time_ = end_time - start_time;
    }
    else {
        sample_end_time_ = stream_->GetDuration();
    }

    XAMP_LOG_INFO("Stream end time {} sec", sample_end_time_);
    Play();

    gapless_play_state_ = GaplessPlayState::INIT;

#ifdef _DEBUG
    Stopwatch sw;
#endif

    stream_task_ = ThreadPool::GetInstance().Run([player = shared_from_this()]() noexcept {
        auto* p = player.get();

        std::unique_lock<std::mutex> lock{ p->pause_mutex_ };

        auto sample_buffer = p->sample_buffer_.Get();
        const auto max_read_sample = p->num_read_sample_;
        const auto num_sample_write = max_read_sample * kMaxWriteRatio;
        AlignPtr<Resampler> resampler;

        XAMP_LOG_DEBUG("max_read_sample: {}, num_sample_write: {}", max_read_sample, num_sample_write);

        try {
            while (p->is_playing_) {
                while (p->is_paused_) {
                    p->pause_cond_.wait(lock);
                }

                if (p->buffer_.GetAvailableWrite() < num_sample_write) {
                    p->wait_timer_.Wait();

                    continue;
                }

                p->ReadSampleLoop(sample_buffer, max_read_sample, lock, resampler);
            }
        }
        catch (const std::exception& e) {
            XAMP_LOG_DEBUG("Stream thread read has exception: {}.", e.what());
        }

        XAMP_LOG_DEBUG("Stream thread end");
    });

#ifdef _DEBUG
    LogTime("Create stream thread time", sw.Elapsed());
#endif
}

}
