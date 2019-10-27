#include <base/logger.h>
#include <base/vmmemlock.h>

#include <output_device/devicefactory.h>
#include <output_device/asiodevicetype.h>
#include <output_device/dsdoutputable.h>

#include <stream/bassfilestream.h>
#include <stream/avfilestream.h>

#include <player/audio_player.h>

namespace xamp::player {

constexpr int32_t BUFFER_STREAM_COUNT = 20;
constexpr std::chrono::milliseconds UPDATE_SAMPLE_INTERVAL(100);

AudioPlayer::AudioPlayer()
    : AudioPlayer(std::weak_ptr<PlaybackStateAdapter>()) {
}

AudioPlayer::AudioPlayer(std::weak_ptr<PlaybackStateAdapter> adapter)
    : is_muted_(false)
    , dsd_mode_(DSDModes::DSD_MODE_PCM)
    , state_(PlayerState::PLAYER_STATE_STOPPED)
    , volume_(0)
    , num_buffer_samples_(0)
    , num_read_sample_(0)
    , read_sample_size_(0)
    , is_playing_(false)
    , is_paused_(false)
    , state_adapter_(adapter) {
#ifdef _WIN32
    VmMemLock::EnableVmMemPrivilege(true);
#endif
}

AudioPlayer::~AudioPlayer() {
	timer_.Stop();
    CloseDevice();
}

void AudioPlayer::Open(const std::wstring& file_path, bool use_bass_stream, const DeviceInfo& device_info) {
    Initial();
    CloseDevice();
    OpenStream(file_path, device_info, use_bass_stream);
    SetDeviceFormat();
    CreateDevice(device_info.device_type_id, device_info.device_id, false);
    OpenDevice();
    CreateBuffer();
    BufferStream();
}

void AudioPlayer::CreateDevice(const ID& device_type_id, const std::wstring& device_id, const bool open_always) {
    if (device_ != nullptr) {
        device_->StopStream();
    }
    if (device_ == nullptr
        || device_id_ != device_id
        || device_type_id_ != device_type_id
        || open_always) {
        if (auto result = DeviceFactory::Instance().Create(device_type_id)) {
            device_type_ = std::move(result.value());
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

void AudioPlayer::OpenStream(const std::wstring& file_path, const DeviceInfo& device_info, bool is_dsd_stream) {
    if (stream_ != nullptr) {
        stream_->Close();
    }

#ifdef ENABLE_FFMPEG
    if (is_dsd_stream) {
        constexpr int32_t MAX_DSD2PCM_SAMPLERATE = 192000;
        stream_ = MakeAlign<FileStream, BassFileStream>();
        if (auto dsd_stream = dynamic_cast<DSDStream*>(stream_.get())) {
            if (!device_info.is_support_dsd) {
                dsd_stream->SetDSDMode(DSDModes::DSD_MODE_PCM);
                dsd_stream->SetPCMSampleRate(MAX_DSD2PCM_SAMPLERATE);
            }
            else {
                dsd_stream->SetDSDMode(DSDModes::DSD_MODE_RAW);
            }
        }
    }
    else {
        stream_ = MakeAlign<FileStream, AvFileStream>();
    }
#else
    if (!stream_) {
        stream_ = MakeAlign<FileStream, BassFileStream>();
    }
#endif
    XAMP_LOG_DEBUG("Use stream type: {}", stream_->GetStreamName());
    stream_->OpenFromFile(file_path);
}

void AudioPlayer::SetState(const PlayerState play_state) {
    if (auto adapter = state_adapter_.lock()) {
        adapter->OnStateChanged(play_state);
    }
    state_ = play_state;
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
            device_->StartStream();
            SetState(PlayerState::PLAYER_STATE_RUNNING);
        }
    }
}

void AudioPlayer::Pause() {
    if (!device_) {
        return;
    }
    XAMP_LOG_DEBUG("Player pasue!");
    if (device_->IsStreamOpen()) {
        is_paused_ = true;
        device_->StopStream();
        SetState(PlayerState::PLAYER_STATE_PAUSED);
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

void AudioPlayer::Stop(bool signal_to_stop, bool shutdown_device) {
    buffer_.Clear();
    if (!device_) {
        return;
    }
    XAMP_LOG_DEBUG("Player stop!");
    if (device_->IsStreamOpen()) {
        CloseDevice();
        if (signal_to_stop) {
            SetState(PlayerState::PLAYER_STATE_STOPPED);
        }
    }
    if (shutdown_device) {
        device_.reset();
        device_id_.clear();
    }
    stream_->Close();
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
        catch (const std::exception& e) {
            XAMP_LOG_DEBUG(e.what());
            Resume();
            return;
        }
        device_->SetStreamTime(stream_time);
        XAMP_LOG_DEBUG("player seeking {} sec.", stream_time);
        std::atomic_exchange(&slice_, AudioSlice{nullptr, 0, stream_time });
        buffer_.Clear();
        buffer_.Fill(0);
        BufferStream();
        Resume();
    }
}

void AudioPlayer::SetVolume(int32_t volume) {
    volume_ = volume;
    if (!device_ || !device_->IsStreamOpen()) {
        return;
    }
    device_->SetVolume(volume);
}

int32_t AudioPlayer::GetVolume() const {
    if (!device_ || !device_->IsStreamOpen()) {
        return volume_;
    }
    return device_->GetVolume();
}

bool AudioPlayer::IsMute() const {
    if (device_ != nullptr && device_->IsStreamOpen()) {
        return device_->IsMuted();
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
        timer_.Start(UPDATE_SAMPLE_INTERVAL, [player]() {
            if (auto p = player.lock()) {
                if (auto adapter = p->state_adapter_.lock()) {
                    if (p->is_paused_) {
                        return;
                    }
                    if (!p->is_playing_) {
                        return;
                    }
                    const auto slice = p->slice_.load();
                    if (p->cache_slice_ == slice) {
                        return;
                    }
                    if (slice.sample_size > 0) {
                        adapter->OnSampleTime(slice.stream_time);
                    } if (p->is_playing_ && slice.sample_size == -1) {
                        p->SetState(PlayerState::PLAYER_STATE_STOPPED);
                        p->is_playing_ = false;
                    }
                }
            }
        });
    }
    buffer_.Clear();
}

std::optional<DeviceInfo> AudioPlayer::GetDefaultDeviceInfo() const {
    if (!device_) {
        return std::nullopt;
    }
    return device_type_->GetDefaultDeviceInfo();
}

double AudioPlayer::GetDuration() const {
    return stream_->GetDuration();
}

PlayerState AudioPlayer::GetState() const noexcept {
    return state_;
}

AudioFormat AudioPlayer::GetStreamFormat() const {
    return stream_->GetFormat();
}

bool AudioPlayer::IsPlaying() const {
    return is_playing_;
}

DSDModes AudioPlayer::GetDSDModes() const {
    return dsd_mode_;
}

void AudioPlayer::CloseDevice() {
    is_playing_ = false;
    is_paused_ = false;
    pause_cond_.notify_all();
    stopped_cond_.notify_all();
    if (device_ != nullptr) {
        if (device_->IsStreamOpen()) {
            device_->StopStream();
            device_->CloseStream();
        }
    }
    if (stream_task_.valid()) {
        stream_task_.get();
    }
}

void AudioPlayer::CreateBuffer() {
    std::atomic_exchange(&slice_, AudioSlice{ nullptr, 0, 0 });

    const AudioFormat input_format(input_format_.GetFormat(),
								   input_format_.GetChannels(),
                                   stream_->GetFormat().GetByteFormat(),
                                   input_format_.GetSampleRate());

    int32_t require_read_sample = 0;

#if ENABLE_ASIO
    if (dsd_mode_ == DSDModes::DSD_MODE_RAW || device_type_->GetTypeId() == ASIODeviceType::Id) {
        require_read_sample = AudioFormat::MAX_SAMPLERATE;
    }
    else {
        require_read_sample = device_->GetBufferSize() * 30;
    }
#else
    require_read_sample = device_->GetBufferSize() * 30;
#endif

    auto output_format = input_format;
    if (require_read_sample != num_read_sample_) {
        auto allocate_size = static_cast<int32_t>(GetPageAlignSize(require_read_sample * stream_->GetSampleSize()))
                * BUFFER_STREAM_COUNT;
        num_buffer_samples_ = allocate_size * 10;
        num_read_sample_ = require_read_sample;
        read_sample_buffer_ = MakeBuffer<int8_t>(allocate_size);
        read_sample_size_ = allocate_size;
    }

    output_format_ = output_format;
    if (buffer_.GetSize() == 0 || buffer_.GetSize() < num_buffer_samples_) {
        vmlock_.UnLock();
        buffer_.Resize(num_buffer_samples_);
        vmlock_.Lock(buffer_.GetData(), buffer_.GetSize());
    }
}

void AudioPlayer::SetDeviceFormat() {
    input_format_ = stream_->GetFormat();
    if (input_format_.GetSampleRate() != output_format_.GetSampleRate()) {
        device_id_.clear();
    }
    output_format_ = input_format_;
}

int AudioPlayer::operator()(void* samples, const int32_t num_buffer_frames, const double stream_time) noexcept {
    const int32_t sample_size = num_buffer_frames * output_format_.GetChannels() * stream_->GetSampleSize();
    if (XAMP_LIKELY( !buffer_.TryRead(reinterpret_cast<int8_t*>(samples), sample_size) )) {
        std::atomic_exchange(&slice_, AudioSlice{ reinterpret_cast<float*>(samples), -1, stream_time });
        stopped_cond_.notify_all();
        return 1;
    }
    std::atomic_exchange(&slice_,
                         AudioSlice{ reinterpret_cast<float*>(samples),
                                     num_buffer_frames * output_format_.GetChannels(),
                                     stream_time });
    return 0;
}

void AudioPlayer::BufferStream() {
    buffer_.Clear();

    for (auto i = 0; i < BUFFER_STREAM_COUNT; ++i) {
        while (true) {
            const auto num_samples = stream_->GetSamples(read_sample_buffer_.get(), num_read_sample_);
            if (num_samples == 0) {
                return;
            }
            if (!ProcessSamples(num_samples)) {
                continue;
            }
            break;
        }
    }
}

bool AudioPlayer::ProcessSamples(int32_t num_samples) noexcept {
    if (stream_->IsDSDFile()) {
        return buffer_.TryWrite(read_sample_buffer_.get(), num_samples);
    }
    return buffer_.TryWrite(read_sample_buffer_.get(), num_samples * sizeof(float));
}

void AudioPlayer::OnError(const Exception& e) noexcept {
    is_playing_ = false;
    XAMP_LOG_DEBUG(e.what());
}

void AudioPlayer::OpenDevice(double stream_time) {
    if (auto dsd_output = dynamic_cast<DSDOutputable*>(device_.get())) {
        if (auto dsd_stream = dynamic_cast<DSDStream*>(stream_.get())) {
            if (dsd_stream->GetDSDMode() == DSDModes::DSD_MODE_RAW) {
                dsd_output->SetIoFormat(IO_FORMAT_DSD);
            }
            else {
                dsd_output->SetIoFormat(IO_FORMAT_PCM);
            }
        }
    }

    device_->OpenStream(output_format_);
    device_->SetStreamTime(stream_time);
}

void AudioPlayer::PlayStream() {
    std::weak_ptr<AudioPlayer> player = shared_from_this();
    stream_task_ = std::async([](std::weak_ptr<AudioPlayer> player) {
         constexpr std::chrono::milliseconds SLEEP_OUTPUT_TIME(500);

         if (auto p = player.lock()) {
            std::unique_lock<std::mutex> lock{ p->pause_mutex_ };

            auto max_read_sample = p->num_read_sample_;
            auto num_sample_write = max_read_sample * 20;

            auto sleep_time = SLEEP_OUTPUT_TIME;

            WaitableTimer wait_timer;
            wait_timer.SetTimeout(sleep_time);

            while (p->is_playing_) {
                while (p->is_paused_) {
                    p->pause_cond_.wait(lock);
                }

                if (p->buffer_.GetAvailableWrite() < num_sample_write) {
                    wait_timer.Wait();
                    continue;
                }

                while (true) {
                    const auto num_samples = p->stream_->GetSamples(p->read_sample_buffer_.get(), max_read_sample);
                    if (num_samples > 0) {
                        if (!p->ProcessSamples(num_samples)) {
                            XAMP_LOG_DEBUG("Process samples failure!");
                            continue;
                        }
                        break;
                    }
                    else {
                        p->stopped_cond_.wait(lock);
                        break;
                    }
                }
            }
        }

        XAMP_LOG_DEBUG("Stream thread finished!");
    }, player);
    Play();
}

}
