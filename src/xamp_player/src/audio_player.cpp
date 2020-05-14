#include <base/str_utilts.h>
#include <base/platform_thread.h>
#include <base/logger.h>
#include <base/threadpool.h>
#include <base/stl.h>

#include <output_device/devicefactory.h>
#include <output_device/asiodevicetype.h>
#include <output_device/dsddevice.h>

#include <stream/bassfilestream.h>
#include <stream/avfilestream.h>
#include <player/soxresampler.h>

#include <player/resampler.h>
#include <player/nullresampler.h>
#include <player/chromaprint.h>
#include <player/audio_player.h>

namespace xamp::player {

constexpr int32_t kBufferStreamCount = 5;
constexpr int32_t kPreallocateBufferSize = 8 * 1024 * 1024;
constexpr int32_t kMaxWriteRatio = 20;
constexpr int32_t kMaxReadRatio = 30;
constexpr std::chrono::milliseconds kUpdateSampleInterval(100);
constexpr std::chrono::seconds kWaitForStreamStopTime(10);
constexpr std::chrono::milliseconds kReadSampleWaitTime(100);

AudioPlayer::AudioPlayer()
    : AudioPlayer(std::weak_ptr<PlaybackStateAdapter>()) {
}

AudioPlayer::AudioPlayer(std::weak_ptr<PlaybackStateAdapter> adapter)
    : is_muted_(false)
    , enable_resample_(false)
    , dsd_mode_(DsdModes::DSD_MODE_PCM)
    , state_(PlayerState::PLAYER_STATE_STOPPED)
    , target_samplerate_(0)
    , volume_(0)
    , num_buffer_samples_(0)
    , num_read_sample_(0)
    , read_sample_size_(0)
    , sample_size_(0)
    , is_playing_(false)
    , is_paused_(false)
    , state_adapter_(adapter) {
    wait_timer_.SetTimeout(kReadSampleWaitTime);
    buffer_.Resize(kPreallocateBufferSize);
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
    ThreadPool::DefaultThreadPool().Stop();
}

void AudioPlayer::UpdateSlice(int32_t sample_size, double stream_time) noexcept {
    std::atomic_exchange_explicit(&slice_,
        AudioSlice{ sample_size, stream_time },
        std::memory_order_relaxed);
}

void AudioPlayer::LoadLib() {
    ThreadPool::DefaultThreadPool();
    BassFileStream::LoadBassLib();    
    AudioDeviceFactory::Instance();
    //Chromaprint::LoadChromaprintLib();
    SoxrResampler::LoadSoxrLib();
}

void AudioPlayer::Open(const std::wstring& file_path, const std::wstring& file_ext, const DeviceInfo& device_info) {
    Initial();
    CloseDevice(true);
    OpenStream(file_path, file_ext, device_info);
    device_info_ = device_info;
}

void AudioPlayer::SetResampler(uint32_t samplerate, AlignPtr<Resampler>&& resampler) {
    target_samplerate_ = samplerate;
    resampler_ = std::move(resampler);
    EnableResampler(true);
}

void AudioPlayer::CreateDevice(const ID& device_type_id, const std::wstring& device_id, const bool open_always) {
    if (device_ == nullptr
        || device_id_ != device_id
        || device_type_id_ != device_type_id
        || open_always) {
        if (auto result = AudioDeviceFactory::Instance().Create(device_type_id)) {
            device_type_ = std::move(result.value());
            device_type_->ScanNewDevice();
            device_ = device_type_->MakeDevice(device_id);
            device_type_id_ = device_type_id;
            device_id_ = device_id;
        }
        else {
            throw DeviceNotFoundException();
        }
    }
    device_->SetAudioCallback(this);
}

bool AudioPlayer::IsDsdStream() const {
    if (!stream_) {
        return false;
    }
    return dynamic_cast<DsdStream*>(stream_.get()) != nullptr;
}

DsdStream* AudioPlayer::AsDsdStream() {
    return dynamic_cast<DsdStream*>(stream_.get());
}

DsdDevice* AudioPlayer::AsDsdDevice() {
    return dynamic_cast<DsdDevice*>(device_.get());
}

AlignPtr<FileStream> AudioPlayer::MakeFileStream(const std::wstring& file_ext) {
    const RobinHoodSet<std::wstring_view> dsd_ext {
        {L".dsf"},
        {L".dff"}
    };
    const RobinHoodSet<std::wstring_view> use_bass {
        {L".m4a"},
        {L".ape"},
        {L".flac"},
    };

    auto is_dsd_stream = dsd_ext.find(file_ext) != dsd_ext.end();
    auto is_use_bass = use_bass.find(file_ext) != use_bass.end();

    if (is_dsd_stream || is_use_bass) {
        return MakeAlign<FileStream, BassFileStream>();
    }
    return MakeAlign<FileStream, AvFileStream>();
}

void AudioPlayer::OpenStream(const std::wstring& file_path, const std::wstring &file_ext, const DeviceInfo& device_info) {
    stream_ = MakeFileStream(file_ext);

    if (auto* dsd_stream = AsDsdStream()) {
        if (AudioDeviceFactory::Instance().IsPlatformSupportedASIO()) {
            if (device_info.is_support_dsd) {
                dsd_stream->SetDSDMode(DsdModes::DSD_MODE_NATIVE);
                dsd_mode_ = DsdModes::DSD_MODE_NATIVE;
            }
            else {
                dsd_stream->SetDSDMode(DsdModes::DSD_MODE_PCM);
                dsd_mode_ = DsdModes::DSD_MODE_PCM;
            }
        }
        else {
            if (device_info.is_support_dsd) {
                dsd_stream->SetDSDMode(DsdModes::DSD_MODE_DOP);
                XAMP_LOG_DEBUG("Use DOP mode");
                dsd_mode_ = DsdModes::DSD_MODE_DOP;
            } else {
                dsd_stream->SetDSDMode(DsdModes::DSD_MODE_PCM);
                XAMP_LOG_DEBUG("Use PCM mode");
                dsd_mode_ = DsdModes::DSD_MODE_PCM;
            }
        }
    }
    else {        
        stream_ = MakeAlign<FileStream, AvFileStream>();
        dsd_mode_ = DsdModes::DSD_MODE_PCM;
    }

    XAMP_LOG_DEBUG("Use stream type: {}", stream_->GetDescription());

    stream_->OpenFromFile(file_path);
}

void AudioPlayer::SetState(const PlayerState play_state) {
    if (auto adapter = state_adapter_.lock()) {
        adapter->OnStateChanged(play_state);
    }
    state_ = play_state;
    XAMP_LOG_DEBUG("Set state: {}", EnumToString(state_));
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
            XAMP_LOG_DEBUG("Volume:{} IsMuted:{}", volume_, is_muted_);
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
        XAMP_LOG_DEBUG("Player pasue!");
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
    XAMP_LOG_DEBUG("Player resume!");
    if (device_->IsStreamOpen()) {
        SetState(PlayerState::PLAYER_STATE_RESUME);
        is_paused_ = false;
        pause_cond_.notify_all();
        stopped_cond_.notify_all();
        SetState(PlayerState::PLAYER_STATE_RUNNING);
        device_->StartStream();
    }
}

void AudioPlayer::Stop(bool signal_to_stop, bool shutdown_device, bool wait_for_stop_stream) {
    if (!device_) {
        return;
    }

    XAMP_LOG_DEBUG("Player stop!");
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

bool AudioPlayer::CanHardwareControlVolume() const {
    if (device_ != nullptr && device_->IsStreamOpen()) {
        return device_->CanHardwareControlVolume();
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

void AudioPlayer::Initial() {
    if (!timer_.IsStarted()) {
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
            } if (p->is_playing_ && slice.sample_size == -1) {
                p->SetState(PlayerState::PLAYER_STATE_STOPPED);
                p->is_playing_ = false;
            }
        });		
    }    
}

std::optional<uint32_t> AudioPlayer::GetDSDSpeed() const {
    if (!stream_) {
        return std::nullopt;
    }

    if (const auto dsd_stream = dynamic_cast<DsdStream*>(stream_.get())) {
        if (dsd_stream->IsDsdFile()) {
            return dsd_stream->GetDsdSpeed();
        }        
    }
    return std::nullopt;
}

std::optional<DeviceInfo> AudioPlayer::GetDefaultDeviceInfo() const {
    if (!device_) {
        return std::nullopt;
    }
    return device_type_->GetDefaultDeviceInfo();
}

double AudioPlayer::GetDuration() const {
    if (!stream_) {
        return 0.0;
    }
    return stream_->GetDuration();
}

PlayerState AudioPlayer::GetState() const noexcept {
    return state_;
}

AudioFormat AudioPlayer::GetStreamFormat() const {
    return stream_->GetFormat();
}

AudioFormat AudioPlayer::GetOutputFormat() const {
    return output_format_;
}

bool AudioPlayer::IsPlaying() const {
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
                XAMP_LOG_DEBUG("Close device failure! {}", e.what());
            }
        }
    }
    if (stream_task_.valid()) {
        XAMP_LOG_DEBUG("Try to stop stream thread!");
        if (stream_task_.wait_for(kWaitForStreamStopTime) == std::future_status::timeout) {
            throw StopStreamTimeoutException();
        }
        XAMP_LOG_DEBUG("Stream thread was finished!");
    }
    buffer_.Clear();
}

void AudioPlayer::CreateBuffer() {
    UpdateSlice();

    uint32_t require_read_sample = 0;

    if (AudioDeviceFactory::Instance().IsPlatformSupportedASIO()) {
        if (dsd_mode_ == DsdModes::DSD_MODE_NATIVE
            || AudioDeviceFactory::Instance().IsASIODevice(device_type_id_)) {
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
        num_buffer_samples_ = allocate_size * 10;
        num_read_sample_ = require_read_sample;
        XAMP_LOG_DEBUG("Allocate interal buffer : {}", FormatBytes(allocate_size));
        sample_buffer_ = MakeBuffer<int8_t>(allocate_size);
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
            XAMP_LOG_DEBUG("Allocate player buffer : {}", FormatBytes(num_buffer_samples_));
        }
    }
    else {
        assert(target_samplerate_ > 0);
        resampler_->Start(input_format_.GetSampleRate(),
                          input_format_.GetChannels(),
                          target_samplerate_,
                          num_read_sample_ / input_format_.GetChannels());
    }
    
    XAMP_LOG_DEBUG("Output device format: {} num_read_sample: {} resampler: {} buffer: {}",
                   output_format_,
                   num_read_sample_,
                   resampler_->GetDescription(),
                   FormatBytes(buffer_.GetSize()));
}

void AudioPlayer::EnableResampler(bool enable) {
    enable_resample_ = enable;
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
        adapter->OnVolumeChange(vol);
        XAMP_LOG_DEBUG("Volum change: {}", vol);
    }
}

int32_t AudioPlayer::OnGetSamples(void* samples, const uint32_t num_buffer_frames, const double stream_time) noexcept {
    const auto num_samples = num_buffer_frames * output_format_.GetChannels();
    const auto sample_size = num_samples * sample_size_;

    if (XAMP_LIKELY( buffer_.TryRead(static_cast<int8_t*>(samples), sample_size) )) {
        UpdateSlice(static_cast<int32_t>(num_samples), stream_time);
        return 0;
    }

    UpdateSlice(-1, stream_time);

    stopped_cond_.notify_all();
    return 1;
}

void AudioPlayer::OnError(const Exception& e) noexcept {
    is_playing_ = false;
    XAMP_LOG_DEBUG(e.what());
}

void AudioPlayer::OnDeviceStateChange(DeviceState state, const std::wstring& device_id) {
    if (auto state_adapter = state_adapter_.lock()) {
        switch (state) {
        case DeviceState::DEVICE_STATE_ADDED:
            XAMP_LOG_DEBUG("Device added device id:{}", ToUtf8String(device_id));
            state_adapter->OnDeviceChanged(DeviceState::DEVICE_STATE_ADDED);
            break;
        case DeviceState::DEVICE_STATE_REMOVED:
            XAMP_LOG_DEBUG("Device removed device id:{}", ToUtf8String(device_id));
            if (device_id == device_id_) {
                state_adapter->OnDeviceChanged(DeviceState::DEVICE_STATE_REMOVED);
            }
            break;
        case DeviceState::DEVICE_STATE_DEFAULT_DEVICE_CHANGE:
            XAMP_LOG_DEBUG("Default device device id:{}", ToUtf8String(device_id));
            state_adapter->OnDeviceChanged(DeviceState::DEVICE_STATE_DEFAULT_DEVICE_CHANGE);
            break;
        }
    }	
}

void AudioPlayer::OpenDevice(double stream_time) {
#ifdef ENABLE_ASIO
    if (auto dsd_output = AsDsdDevice()) {
        if (const auto dsd_stream = AsDsdStream()) {
            if (dsd_stream->GetDsdMode() == DsdModes::DSD_MODE_NATIVE) {
                dsd_output->SetIoFormat(AsioIoFormat::IO_FORMAT_DSD);
                dsd_mode_ = DsdModes::DSD_MODE_NATIVE;
            }
            else {
                dsd_output->SetIoFormat(AsioIoFormat::IO_FORMAT_PCM);
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

void AudioPlayer::Seek(double stream_time) {
    if (!device_) {
        return;
    }
    if (device_->IsStreamOpen()) {
        Pause();
        try {
            stream_->Seek(stream_time);
        }
        catch (const std::exception & e) {
            XAMP_LOG_DEBUG(e.what());
            Resume();
            return;
        }
        device_->SetStreamTime(stream_time);
        XAMP_LOG_DEBUG("player seeking {} sec.", stream_time);
        UpdateSlice(0, stream_time);
        buffer_.Clear();
        BufferStream();
        Resume();
    }
}

void AudioPlayer::BufferStream() {
    buffer_.Clear();

    auto* const sample_buffer = sample_buffer_.get();
    sample_size_ = stream_->GetSampleSize();

    if (enable_resample_) {
        resampler_->Flush();
    }

    for (auto i = 0; i < kBufferStreamCount; ++i) {
        while (true) {
	        const auto num_samples = stream_->GetSamples(sample_buffer, num_read_sample_);
            if (num_samples == 0) {
                return;
            }
            if (!resampler_->Process(reinterpret_cast<const float*>(sample_buffer_.get()), num_samples, buffer_)) {
                continue;
            }
            break;
        }
    }
}

void AudioPlayer::ReadSampleLoop(int8_t *sample_buffer, uint32_t max_read_sample, std::unique_lock<std::mutex>& lock) {
    while (is_playing_) {
	    const auto num_samples = stream_->GetSamples(sample_buffer, max_read_sample);

        if (num_samples > 0) {
            if (!resampler_->Process(reinterpret_cast<const float*>(sample_buffer_.get()), num_samples, buffer_)) {
                continue;
            }
        }
        else {
            XAMP_LOG_DEBUG("Finish read all samples, wait for finish!");
            stopped_cond_.wait(lock);
        }
        break;
    }
}

void AudioPlayer::StartPlay() {
    SetDeviceFormat();
    CreateDevice(device_info_.device_type_id, device_info_.device_id, false);
    OpenDevice();
    CreateBuffer();
    BufferStream();

    // 預先啟動output device開始撥放, 因有預先塞入資料可以加速撥放效果.
    Play();

    stream_task_ = ThreadPool::DefaultThreadPool().StartNew([player = shared_from_this()]() noexcept {
	    auto* p = player.get();
        std::unique_lock<std::mutex> lock{ p->pause_mutex_ };

        auto sample_buffer = player->sample_buffer_.get();
        const auto max_read_sample = p->num_read_sample_;
        const auto num_sample_write = max_read_sample * kMaxWriteRatio;

        while (p->is_playing_) {
            while (p->is_paused_) {
                p->pause_cond_.wait(lock);
            }

            if (p->buffer_.GetAvailableWrite() < num_sample_write) {
                p->wait_timer_.Wait();
                continue;
            }

            try {
                p->ReadSampleLoop(sample_buffer, max_read_sample, lock);
            }
            catch (const std::exception& e) {
                XAMP_LOG_DEBUG("Stream read has exception: {}.", e.what());
                break;
            }
        }
    });
}

}
