#include <filesystem>

#include <base/str_utilts.h>
#include <base/platform_thread.h>
#include <base/logger.h>
#include <base/vmmemlock.h>
#include <base/str_utilts.h>

#include <output_device/devicefactory.h>
#include <output_device/asiodevicetype.h>
#include <output_device/dsddevice.h>

#include <stream/bassfilestream.h>
#include <stream/avfilestream.h>

#ifdef _WIN32
#include <player/cdspresampler.h>
#endif

#include <player/soxresampler.h>

#include <player/resampler.h>
#include <player/chromaprint.h>
#include <player/audio_player.h>

namespace xamp::player {

constexpr int32_t BUFFER_STREAM_COUNT = 5;
constexpr int32_t PREALLOCATE_BUFFER_SIZE = 8 * 1024 * 1024;
constexpr int32_t MAX_WRITE_RATIO = 20;
constexpr int32_t MAX_READ_RATIO = 30;
constexpr int32_t MAX_READ_SAMPLE = 32768 * 2;
constexpr std::chrono::milliseconds UPDATE_SAMPLE_INTERVAL(30);
constexpr std::chrono::seconds WAIT_FOR_STRAEM_STOP_TIME(5);
constexpr std::chrono::milliseconds SLEEP_OUTPUT_TIME(100);

AudioPlayer::AudioPlayer()
    : AudioPlayer(std::weak_ptr<PlaybackStateAdapter>()) {
}

AudioPlayer::AudioPlayer(std::weak_ptr<PlaybackStateAdapter> adapter)
    : is_muted_(false)
    , enable_resample_(false)
    , dsd_mode_(DsdModes::DSD_MODE_PCM)
    , state_(PlayerState::PLAYER_STATE_STOPPED)
    , target_samplerate_(-1)
    , volume_(0)
    , num_buffer_samples_(0)
    , num_read_sample_(0)
    , read_sample_size_(0)
    , sample_size_(0)
    , is_playing_(false)
    , is_paused_(false)
    , state_adapter_(adapter) {
	PrepareAllocate();
}

AudioPlayer::~AudioPlayer() {
	timer_.Stop();
	try {
		CloseDevice(true);
	}
	catch (...) {
	}
}

void AudioPlayer::LoadLib() {
    BassFileStream::LoadBassLib();
#ifdef _WIN32
    SoxrResampler::LoadSoxrLib();
    CdspResampler::LoadCdspLib();
    Chromaprint::LoadChromaprintLib();
#endif
}

void AudioPlayer::PrepareAllocate() {
	wait_timer_.SetTimeout(SLEEP_OUTPUT_TIME);
	buffer_.Resize(PREALLOCATE_BUFFER_SIZE);
    // Initial std::async internal threadpool
    stream_task_ = std::async(std::launch::async | std::launch::deferred, []() {});
}

void AudioPlayer::Open(const std::wstring& file_path, const std::wstring& file_ext, const DeviceInfo& device_info) {
    Initial();
    CloseDevice(true);
    OpenStream(file_path, file_ext, device_info);
    SetDeviceFormat();
    CreateDevice(device_info.device_type_id, device_info.device_id, false);
    OpenDevice();
    CreateBuffer();
    BufferStream();
}

void AudioPlayer::SetResampler(int32_t samplerate, AlignPtr<Resampler>&& resampler) {
    target_samplerate_ = samplerate;
    resampler_ = std::move(resampler);
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

FileStream* AudioPlayer::AsFileStream() {
    return dynamic_cast<FileStream*>(stream_.get());
}

DsdDevice* AudioPlayer::AsDsdDevice() {
    return dynamic_cast<DsdDevice*>(device_.get());
}

void AudioPlayer::OpenStream(const std::wstring& file_path, const std::wstring &file_ext, const DeviceInfo& device_info) {
    if (stream_ != nullptr) {
        stream_->Close();
    }

    static constexpr const std::wstring_view dsd_ext(L".dsf,.dff");
    static constexpr const std::wstring_view use_bass(L".m4a,.ape");
    auto is_dsd_stream = dsd_ext.find(file_ext) != std::wstring_view::npos;
    auto is_use_bass = use_bass.find(file_ext) != std::wstring_view::npos;

#ifdef ENABLE_FFMPEG
    if (is_dsd_stream || is_use_bass) {
		auto is_support_dsd_file = false;
		if (stream_ != nullptr) {
			if (IsDsdStream()) {
				is_support_dsd_file = true;
			}
		}
		if (!is_support_dsd_file) {
			stream_ = MakeAlign<AudioStream, BassFileStream>();
		}
        if (auto dsd_stream = AsDsdStream()) {
#ifdef _WIN32
            if (device_info.is_support_dsd && is_dsd_stream) {
                dsd_stream->SetDSDMode(DsdModes::DSD_MODE_NATIVE);
                dsd_mode_ = DsdModes::DSD_MODE_NATIVE;
            } else {                
                dsd_stream->SetDSDMode(DsdModes::DSD_MODE_PCM);
                dsd_mode_ = DsdModes::DSD_MODE_PCM;
            }
#else
            dsd_stream->SetDSDMode(DsdModes::DSD_MODE_DOP);
            XAMP_LOG_DEBUG("Use DOP mode");
            dsd_mode_ = DsdModes::DSD_MODE_DOP;
#endif
        }
    }
    else {        
        stream_ = MakeAlign<AudioStream, AvFileStream>();
        dsd_mode_ = DsdModes::DSD_MODE_PCM;
    }
#else
    if (!stream_) {
        stream_ = MakeAlign<FileStream, BassFileStream>();
    }
#endif
    XAMP_LOG_DEBUG("Use stream type: {}", stream_->GetDescription());

	if (auto file_stream = AsFileStream()) {
		file_stream->OpenFromFile(file_path);
	}
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
                    if (slice.sample_size > 0) {
                        adapter->OnSampleTime(slice.stream_time);
                    } if (p->is_playing_ && slice.sample_size == -1) {
                        p->SetState(PlayerState::PLAYER_STATE_STOPPED);
                        p->is_playing_ = false;
                    }
                    std::vector<float> buffer(4096 * 2);
                    if (p->analysis_buffer_.TryRead(buffer.data(), buffer.size())) {
                        p->analysis_->Process(buffer);
                    }
                }
            }
        });		
    }    
}

std::optional<int32_t> AudioPlayer::GetDSDSpeed() const {
    if (!stream_) {
        return std::nullopt;
    }

    if (auto dsd_stream = dynamic_cast<DsdStream*>(stream_.get())) {
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

bool AudioPlayer::IsPlaying() const {
    return is_playing_;
}

DsdModes AudioPlayer::GetDSDModes() const {
    return dsd_mode_;
}

void AudioPlayer::CloseDevice(bool wait_for_stop_stream) {
    is_playing_ = false;
    is_paused_ = false;
    pause_cond_.notify_all();
    stopped_cond_.notify_all();
    if (device_ != nullptr) {
        if (device_->IsStreamOpen()) {
            try {
                device_->StopStream(wait_for_stop_stream);
                device_->CloseStream();
            } catch (const Exception &e) {
                XAMP_LOG_DEBUG("Close device failure! {}", e.what());
            }
        }
    }
    if (stream_task_.valid()) {
        if (stream_task_.wait_for(WAIT_FOR_STRAEM_STOP_TIME) == std::future_status::timeout) {
            throw StopStreamTimeoutException();
        }
    }
    buffer_.Clear();
}

void AudioPlayer::CreateBuffer() {
    std::atomic_exchange(&slice_, AudioSlice{ 0, 0 });

    int32_t require_read_sample = 0;

    if (DeviceFactory::Instance().IsPlatformSupportedASIO()) {
        if (dsd_mode_ == DsdModes::DSD_MODE_NATIVE
                || DeviceFactory::Instance().IsASIODevice(device_type_id_)) {
			require_read_sample = XAMP_MAX_SAMPLERATE;
		}
		else {
			require_read_sample = device_->GetBufferSize() * MAX_READ_RATIO;
		}
	}
	else {
        require_read_sample = device_->GetBufferSize() * MAX_READ_RATIO;
	}
    
    if (require_read_sample != num_read_sample_) {
        auto allocate_size = require_read_sample * stream_->GetSampleSize() * BUFFER_STREAM_COUNT;
        num_buffer_samples_ = allocate_size * 10;
        num_read_sample_ = require_read_sample;
        sample_buffer_ = MakeBuffer<int8_t>(allocate_size);
        read_sample_size_ = allocate_size;
    }

    std::string_view resampler_desc = "None";

    if (!enable_resample_) {
        if (buffer_.GetSize() == 0 || buffer_.GetSize() < num_buffer_samples_) {
            XAMP_LOG_DEBUG("Buffer too small reallocate.");
            buffer_.Resize(num_buffer_samples_);
            XAMP_LOG_DEBUG("Allocate memory size:{}", buffer_.GetSize());
        }
    }
    else {
        resampler_->Start(input_format_.GetSampleRate(),
            input_format_.GetChannels(),
            target_samplerate_,
            num_read_sample_ / input_format_.GetChannels());  
        resampler_desc = resampler_->GetDescription();
    }

    analysis_buffer_.Resize(4096 * output_format_.GetChannels() * 4);

    XAMP_LOG_DEBUG("Output device format: {} num_read_sample: {} Resampler: {}",
        output_format_,
        num_read_sample_,
        resampler_desc);
}

void AudioPlayer::SetEnableResampler(bool enable) {
    enable_resample_ = enable;
}

void AudioPlayer::SetDeviceFormat() {
    input_format_ = stream_->GetFormat();

    if (enable_resample_) {
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

int AudioPlayer::OnGetSamples(void* samples, const int32_t num_buffer_frames, const double stream_time) noexcept {
    const int32_t num_samples = num_buffer_frames * output_format_.GetChannels();
    const int32_t sample_size = num_samples * sample_size_;

    if (XAMP_LIKELY( buffer_.TryRead(reinterpret_cast<int8_t*>(samples), sample_size) )) {
        std::atomic_exchange(&slice_,
                             AudioSlice{ num_samples, stream_time });
        analysis_buffer_.TryWrite(reinterpret_cast<float*>(samples), num_samples);
        return 0;
    }

    std::atomic_exchange(&slice_, AudioSlice{ -1, stream_time });

    stopped_cond_.notify_all();
    return 1;
}

bool AudioPlayer::FillSamples(int32_t num_samples) noexcept {
    if (GetDSDModes() == DsdModes::DSD_MODE_NATIVE) {
        return buffer_.TryWrite(sample_buffer_.get(), num_samples);
    }
    return buffer_.TryWrite(sample_buffer_.get(), num_samples * sample_size_);
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
            state_adapter->OnDeviceChanged();
            break;
        case DeviceState::DEVICE_STATE_REMOVED:
            XAMP_LOG_DEBUG("Device removed device id:{}", ToUtf8String(device_id));
            if (device_id == device_id_) {
                // TODO: thread safe?
                Stop(true, true, true);
                state_adapter->OnDeviceChanged();
            }
            break;
        case DeviceState::DEVICE_STATE_DEFAULT_DEVICE_CHANGE:
            XAMP_LOG_DEBUG("Default device device id:{}", ToUtf8String(device_id));
            state_adapter->OnDeviceChanged();
            break;
        }
    }	
}

void AudioPlayer::OpenDevice(double stream_time) {
#ifdef ENABLE_ASIO
    if (auto dsd_output = AsDsdDevice()) {
        if (auto dsd_stream = AsDsdStream()) {
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
        std::atomic_exchange(&slice_, AudioSlice{ 0, stream_time });
        buffer_.Clear();
        buffer_.Fill(0);
        BufferStream();
        Resume();
    }
}

void AudioPlayer::BufferStream() {
    buffer_.Clear();

    auto sample_buffer = sample_buffer_.get();

    sample_size_ = stream_->GetSampleSize();

    for (auto i = 0; i < BUFFER_STREAM_COUNT; ++i) {
        while (true) {
            const auto num_samples = stream_->GetSamples(sample_buffer, num_read_sample_);
            if (num_samples == 0) {
                return;
            }

            if (enable_resample_) {
                if (!resampler_->Process((const float*)sample_buffer_.get(), num_samples, buffer_)) {
                    continue;
                }
            }
            else {
                if (!FillSamples(num_samples)) {
                    XAMP_LOG_ERROR("Buffer is full.");
                    continue;
                }
            }
            break;
        }
    }
}

void AudioPlayer::ReadSampleLoop(int32_t max_read_sample, std::unique_lock<std::mutex>& lock) {
    auto resampler = resampler_.get();
    auto sample_buffer = sample_buffer_.get();

    while (is_playing_) {
        const auto num_samples = stream_->GetSamples(sample_buffer, max_read_sample);

        if (num_samples > 0) {
            if (enable_resample_) {
                if (!resampler->Process((const float*)sample_buffer, num_samples, buffer_)) {
                    continue;
                }
            } else {
                if (!FillSamples(num_samples)) {
                    XAMP_LOG_DEBUG("Process samples failure!");
                    continue;
                }
            }
            break;
        }
        else {
            stopped_cond_.wait(lock);
            break;
        }
    }
}

void AudioPlayer::PlayStream() {
    std::weak_ptr<AudioPlayer> player = shared_from_this();

    // 1.預先啟動output device開始撥放, 因有預先塞入資料可以加速撥放效果.
    // 2.std::async啟動會比較慢一點.
	Play();

    constexpr auto stream_proc = [](std::weak_ptr<AudioPlayer> player) noexcept {
        if (auto shared_this = player.lock()) {
            auto p = shared_this.get();
            std::unique_lock<std::mutex> lock{ p->pause_mutex_ };

            auto max_read_sample = p->num_read_sample_;
            auto num_sample_write = max_read_sample * MAX_WRITE_RATIO;

#ifdef _DEBUG
            SetThreadName("Streaming thread");
#endif

            while (p->is_playing_) {
                while (p->is_paused_) {
                    p->pause_cond_.wait(lock);
                }

                if (p->buffer_.GetAvailableWrite() < num_sample_write) {
                    p->wait_timer_.Wait();
                    continue;
                }

                try {
                    p->ReadSampleLoop(max_read_sample, lock);
                }
                catch (const std::exception & e) {
                    XAMP_LOG_DEBUG("Stream read has exception: {}.", e.what());
                    break;
                }
            }

            p->stream_->Close();         
        }

        XAMP_LOG_DEBUG("Stream thread finished!");
    };

    stream_task_ = std::async(std::launch::async | std::launch::deferred, stream_proc, player);
}

}
