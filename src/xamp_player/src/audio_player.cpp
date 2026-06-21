#include <base/str_utilts.h>
#include <base/platform.h>
#include <base/logger.h>
#include <base/stl.h>
#include <base/threadpool.h>
#include <base/dsdsampleformat.h>
#include <base/buffer.h>
#include <base/timer.h>
#include <base/scopeguard.h>
#include <base/waitabletimer.h>
#include <base/stopwatch.h>
#include <base/executor.h>
#include <base/trackinfo.h>

#include <output_device/api.h>
#ifdef XAMP_OS_WIN
#include <output_device/win32/asiodevicetype.h>
#endif
#include <output_device/idsddevice.h>
#include <output_device/iaudiodevicemanager.h>

#include <stream/api.h>
#include <stream/dspmanager.h>
#include <stream/iaudiostream.h>
#include <stream/idsdstream.h>
#include <stream/filestream.h>
#include <stream/iaudioprocessor.h>
#include <stream/bassfilestream.h>
#include <stream/r8brainresampler.h>

#include <player/iplaybackstateadapter.h>
#include <player/audio_player.h>

XAMP_AUDIO_PLAYER_NAMESPACE_BEGIN
namespace {
    XAMP_DECLARE_LOG_NAME(AudioPlayer);

    constexpr int32_t kBufferStreamCount = 6;
    // 1MB
    constexpr uint32_t kPreallocateBufferSize = 1 * 1024 * 1024;
    // 32MB
    constexpr uint32_t kMaxPreAllocateBufferSize = 32 * 1024 * 1024;
    // 8KB
    constexpr uint32_t kMinReadBufferSize = 8192;

    constexpr int32_t  kTotalBufferStreamCount = 8;
    constexpr uint32_t kMaxWriteRatio = 20;
    constexpr uint32_t kMaxReadRatio = 4;
    constexpr uint32_t kMaxBufferSecs = 3;
    constexpr uint32_t kResamplerOutputPaddingSamples = 4096;

    constexpr std::chrono::milliseconds kUpdateSampleIntervalMs(15);
    constexpr std::chrono::milliseconds kReadSampleWaitTimeMs(15);
    constexpr std::chrono::milliseconds kPauseWaitTimeout(10);
    constexpr std::chrono::seconds kWaitForStreamStopTime(10);
    constexpr std::chrono::seconds kWaitForSignalWhenReadFinish(3);
    constexpr std::chrono::milliseconds kMinimalCopySamplesTime(5);    

    int32_t GetBufferCount(int32_t sample_rate) {
        return sample_rate > (176400 * 2) ? kBufferStreamCount : 3;
    }

#if defined(XAMP_OS_WIN)
    IDsdDevice* AsDsdDevice(ScopedPtr<IOutputDevice> const& device) {
        return dynamic_cast<IDsdDevice*>(device.get());
    }
#endif
}

AudioPlayer::AudioPlayer(
    const std::shared_ptr<IThreadPool>& playback_thread_pool,
    const std::shared_ptr<IThreadPool>& player_thread_pool)
    : is_muted_(false)
	, is_dsd_file_(false)
    , num_read_buffer_size_(0)
    , num_write_buffer_size_(0)
    , min_fifo_write_size_(0)
    , sample_end_time_(0)
    , dsp_manager_(StreamFactory::makeDSPManager())
    , device_manager_(MakeAudioDeviceManager())
    , logger_(XampLoggerFactory.getLogger(XAMP_LOG_NAME(AudioPlayer)))
	, fifo_(alignUp(kPreallocateBufferSize, getPageSize()))
	, playback_thread_pool_(playback_thread_pool)
	, player_thread_pool_(player_thread_pool) {
    PreventSleep(true);
}

AudioPlayer::~AudioPlayer() {
    destroy();
}

void AudioPlayer::destroy() {
    XAMP_LOG_D(logger_, "destroy audio player.");

    timer_.stop();
    try {
        closeDevice(true, true);
        stop(false, true);
    }
    catch (...) {
    }    
    file_stream_.reset();
    read_buffer_.reset();
#if defined(XAMP_OS_WIN)
    ResetAsioDriver();
#endif

    PreventSleep(false);
    freeAvLib();

    device_.reset();
    device_manager_.reset();
}

void AudioPlayer::openArchiveEntry(ArchiveEntry archive_entry,
    const DeviceInfo& device_info,
    uint32_t target_sample_rate, 
    DsdModes output_mode) {
    closeDevice(true);
    updatePlayerStreamTime();
    openStream(std::move(archive_entry), output_mode);
    device_info_ = device_info;
    audio_config_.target_sample_rate = target_sample_rate;
}

void AudioPlayer::open(ScopedPtr<FileStream> file_stream,
    const DeviceInfo& device_info,
    uint32_t target_sample_rate,
    DsdModes output_mode) {
    closeDevice(true);
    updatePlayerStreamTime();
    openStream(std::move(file_stream), output_mode);
    device_info_ = device_info;
    audio_config_.target_sample_rate = target_sample_rate;
}

void AudioPlayer::createDevice(const Uuid& device_type_id,
    const std::string & device_id, bool open_always) {
    if (device_ == nullptr
        || device_id_ != device_id
        || device_type_id_ != device_type_id
        || open_always) {
        if (device_type_id_ != device_type_id) {
            // ASIO drivers may be unloaded after the device type changes.
            device_.reset();
            ResetAsioDriver();
            XAMP_LOG_D(logger_, "ResetASIODriver!");
        }    	
        device_type_ = device_manager_->create(device_type_id);
        device_type_->scanNewDevice();
        device_ = device_type_->makeDevice(playback_thread_pool_, device_id);
        device_type_id_ = device_type_id;
        device_id_ = device_id;
        XAMP_LOG_D(logger_, "create device: {}", device_type_->getDescription());
    }
    device_->setAudioCallback(this);
}

bool AudioPlayer::isDsdFile() const {
    return is_dsd_file_;
}

void AudioPlayer::readStreamInfo(DsdModes dsd_mode, 
    const ScopedPtr<FileStream>& stream) {
    audio_config_.dsd_mode = dsd_mode;

    playback_state_.stream_duration = stream->getDuration();
    input_format_ = stream->getFormat();

    if (dsd_mode == DsdModes::DSD_MODE_PCM) {
        dsd_speed_ = std::nullopt;
        is_dsd_file_ = false;
        return;
    }

    if (const auto* dsd_stream = asDsdStream(stream)) {
        if (!dsd_stream->isDsdFile()) {
            return;
        }
        is_dsd_file_ = true;
        dsd_speed_ = dsd_stream->getDsdSpeed();
    } else {
        is_dsd_file_ = false;
        dsd_speed_ = std::nullopt;
    }
}

void AudioPlayer::openStream(ArchiveEntry archive_entry, DsdModes dsd_mode) {
    file_stream_ = StreamFactory::makeFileStream(std::move(archive_entry), 
        dsd_mode);

    readStreamInfo(dsd_mode, file_stream_);
    XAMP_LOG_D(logger_, "open stream type: {} {} duration:{:.2f} sec.",
        file_stream_->getDescription(),
        enumToString(audio_config_.dsd_mode),
        playback_state_.stream_duration);
}

void AudioPlayer::openStream(ScopedPtr<FileStream> file_stream, DsdModes dsd_mode) {
    failWith<Exception>(file_stream != nullptr, "File stream is null.");

    file_stream_ = std::move(file_stream);

    readStreamInfo(dsd_mode, file_stream_);
    XAMP_LOG_D(logger_, "open stream type: {} {} duration:{:.2f} sec.",
        file_stream_->getDescription(),
        enumToString(audio_config_.dsd_mode),
        playback_state_.stream_duration);
}

void AudioPlayer::setState(const PlayerState play_state) {
    if (const auto adapter = state_adapter_.lock()) {
        adapter->onStateChanged(play_state);
    }
    playback_state_.state = play_state;
    XAMP_LOG_D(logger_, "Set state: {}.", enumToString(playback_state_.state));
}

void AudioPlayer::pause() {
    if (!device_) {
        return;
    }

    XAMP_LOG_D(logger_, "Player pause.");
    if (!playback_state_.is_paused) {
        if (device_->isStreamOpen()) {
            playback_state_.is_paused = true;
            device_->stopStream(false);
            setState(PlayerState::PLAYER_STATE_PAUSED);            
        }
    }
}

void AudioPlayer::resume() {
    if (!device_) {
        return;
    }

    XAMP_LOG_D(logger_, "Player resume.");
    if (device_->isStreamOpen()) {
        setState(PlayerState::PLAYER_STATE_RESUME);
        playback_state_.is_paused = false;
        pause_cond_.notify_all();
        read_finish_and_wait_seek_signal_cond_.notify_all();
        device_->startStream();
        setState(PlayerState::PLAYER_STATE_RUNNING);
    }
}

void AudioPlayer::stop(bool signal_to_stop,
    bool shutdown_device, 
    bool wait_for_stop_stream) {
    if (!device_) {
        return;
    }

    XAMP_LOG_D(logger_, "Player stop.");
    if (device_->isStreamOpen()) {
        XAMP_LOG_D(logger_, "close device.");
        closeDevice(wait_for_stop_stream);
        updatePlayerStreamTime();
        if (signal_to_stop) {
            setState(PlayerState::PLAYER_STATE_USER_STOPPED);                        
        }
    }

    if (shutdown_device) {
        XAMP_LOG_D(logger_, "shutdown device.");
        if (IsAsioDevice(device_type_id_)) {
            device_.reset();
            ResetAsioDriver();           
        }
        device_id_.clear();
        device_.reset();
    }
    {
        std::lock_guard<FastMutex> stream_lock{ stream_mutex_ };
        file_stream_.reset();
    }
    fifo_.clear();
}

void AudioPlayer::setVolume(uint32_t volume) {
    audio_config_.volume = volume;
    if (!device_ || !device_->isStreamOpen()) {
        return;
    }
    device_->setVolume(volume);
}

uint32_t AudioPlayer::getVolume() const {
    if (!device_ || !device_->isStreamOpen()) {
        return audio_config_.volume;
    }
    return device_->getVolume();
}

bool AudioPlayer::isHardwareControlVolume() const {
    //if (device_ != nullptr && device_->isStreamOpen()) {
    //    return device_->isHardwareControlVolume();
    //}
    return false;
}

bool AudioPlayer::isMute() const {
    return is_muted_;
}

void AudioPlayer::setMute(bool mute) {
    is_muted_ = mute;
    if (!device_ || !device_->isStreamOpen()) {
        return;
    }
    device_->setMute(mute);
}

std::optional<uint32_t> AudioPlayer::getDsdSpeed() const {
    return dsd_speed_;	
}

double AudioPlayer::getDuration() const {
    if (!file_stream_) {
        return 0.0;
    }
    return playback_state_.stream_duration;
}

PlayerState AudioPlayer::getState() const {
    return playback_state_.state;
}

AudioFormat AudioPlayer::getInputFormat() const {
    auto file_format = input_format_;
    file_format.setBitPerSample(file_stream_->getBitDepth());
    return file_format;
}

AudioFormat AudioPlayer::getOutputFormat() const {
    return output_format_;
}

bool AudioPlayer::isPlaying() const {
    return playback_state_.is_playing;
}

DsdModes AudioPlayer::getDsdModes() const {
    return audio_config_.dsd_mode;
}

void AudioPlayer::closeDevice(bool wait_for_stop_stream, bool quit) {
    playback_state_.is_playing = false;
    playback_state_.is_paused = false;
    pause_cond_.notify_all();
    read_finish_and_wait_seek_signal_cond_.notify_all();

    if (stream_task_.valid()) {
        XAMP_LOG_D(logger_, "Try to stop stream thread.");
        stream_task_.get();
        stream_task_ = Future<void>();
        XAMP_LOG_D(logger_, "Stream thread was finished.");
    }

    playback_state_.stream_offset_time = 0;

    if (device_ != nullptr) {
        if (device_->isStreamOpen()) {
            XAMP_LOG_D(logger_, "stop output device");
            try {
                device_->stopStream(wait_for_stop_stream);
                device_->closeStream();
			}
			catch (const Exception& e) {
				XAMP_LOG_D(logger_, "close device failure. {}", e.what());
			}
		}
	}

    fifo_.clear();

}

void AudioPlayer::resizeReadBuffer(uint32_t allocate_size) {
    if (read_buffer_.getSize() == 0
        || read_buffer_.getSize() != allocate_size) {
        XAMP_LOG_D(logger_, "Allocate read buffer : {}.", 
            String::formatBytes(allocate_size));
        read_buffer_ = makeBuffer<std::byte>(allocate_size);
    }
}

void AudioPlayer::resizeFIFO(uint32_t fifo_size) {
    if (fifo_.size() == 0 || fifo_.size() < fifo_size) {
        XAMP_LOG_D(logger_, "Allocate fifo buffer : {}.",
            String::formatBytes(fifo_size));
        fifo_.resize(fifo_size);
    }
	fifo_.clear();
}

void AudioPlayer::createBuffer() {
    const uint32_t page_size = static_cast<uint32_t>(getPageSize());
    const uint32_t device_buffer_samples = device_->getBufferSize();
    const uint32_t file_sample_size = file_stream_->getSampleSize();
    const uint32_t output_bytes_per_sec = output_format_.getAvgBytesPerSec();

    auto multiply = [](uint32_t value, uint32_t factor, uint32_t limit = std::numeric_limits<uint32_t>::max()) {
        if (factor == 0) {
            return 0U;
        }
        if (value > limit / factor) {
            return limit;
        }
        return value * factor;
        };

    auto align_page_size = [page_size](uint32_t size) {
        const uint32_t max_size = std::numeric_limits<uint32_t>::max() - page_size;
        return alignUp(std::min(size, max_size), page_size);
        };

    auto ceil_ratio = [](uint32_t numerator, uint32_t denominator) {
        if (numerator == 0 || denominator == 0) {
            return 1U;
        }
        return std::max(((numerator - 1) / denominator) + 1, 1U);
        };

    auto cap_preallocate_size = [](uint32_t size) {
        return std::min(size, kMaxPreAllocateBufferSize);
        };

    uint32_t read_buffer_size = 0;
    uint32_t fifo_size = 0;
    min_fifo_write_size_ = 0;

    if (audio_config_.dsd_mode == DsdModes::DSD_MODE_NATIVE) {
        // Native DSD is byte-oriented: one stream sample is one byte in the reader and FIFO.
        num_read_buffer_size_ = align_page_size(output_format_.getSampleRate() / 8);
        num_write_buffer_size_ = multiply(device_buffer_samples, kMaxBufferSecs);
        min_fifo_write_size_ = num_read_buffer_size_;
        read_buffer_size = multiply(num_read_buffer_size_, file_sample_size);
        fifo_size = align_page_size(multiply(output_bytes_per_sec, kMaxBufferSecs));
    }
    else {
        const uint32_t input_bytes_per_sec = input_format_.getAvgBytesPerSec();
        const uint32_t output_to_input_ratio = ceil_ratio(output_bytes_per_sec, input_bytes_per_sec);

        // Keep reads large enough for file/DSP efficiency, while the FIFO watermark is measured in bytes.
        num_read_buffer_size_ = std::max(
            align_page_size(multiply(device_buffer_samples, kMaxReadRatio)),
            kMinReadBufferSize);
        num_write_buffer_size_ = align_page_size(
            multiply(multiply(device_buffer_samples, output_to_input_ratio), sizeof(float)));
        min_fifo_write_size_ = align_page_size(estimateDspOutputBytes(num_read_buffer_size_));

        const uint32_t preferred_fifo_size = multiply(output_bytes_per_sec, kMaxBufferSecs);
        const uint32_t min_fifo_headroom = multiply(
            (std::max)(num_write_buffer_size_, min_fifo_write_size_),
            static_cast<uint32_t>(GetBufferCount(output_format_.getSampleRate())));
        fifo_size = align_page_size(cap_preallocate_size(
            (std::max)(preferred_fifo_size, min_fifo_headroom)));

        const uint32_t min_read_buffer_size = multiply(num_read_buffer_size_, file_sample_size);
        const uint32_t preferred_read_buffer_size = multiply(
            multiply(num_write_buffer_size_, file_sample_size, kMaxPreAllocateBufferSize),
            kTotalBufferStreamCount,
            kMaxPreAllocateBufferSize);
        read_buffer_size = align_page_size(std::max(min_read_buffer_size,
            cap_preallocate_size(preferred_read_buffer_size)));
    }

    resizeReadBuffer(read_buffer_size);
    resizeFIFO(fifo_size);

    XAMP_LOG_DEBUG(
        "Device output buffer:{} fifo write watermark:{} min fifo write:{} read samples:{} read buffer:{} fifo buffer:{}.",
        String::formatBytes(device_buffer_samples),
        String::formatBytes(num_write_buffer_size_),
        String::formatBytes(min_fifo_write_size_),
        num_read_buffer_size_,
        String::formatBytes(read_buffer_size),
        String::formatBytes(fifo_size));
}

void AudioPlayer::setDeviceFormat() {
    if (audio_config_.target_sample_rate != 0 
        && isPcmAudio(audio_config_.dsd_mode)) {
        if (output_format_.getSampleRate() != audio_config_.target_sample_rate) {
            device_id_.clear();
        }
        output_format_ = input_format_;
        output_format_.setSampleRate(audio_config_.target_sample_rate);
    }
    else {
        if (input_format_.getSampleRate() != output_format_.getSampleRate()) {
            device_id_.clear();
        }
        output_format_ = input_format_;
    }
}

void AudioPlayer::onVolumeChange(int32_t vol) {
    if (const auto adapter = state_adapter_.lock()) {
        adapter->onVolumeChanged(vol);
        XAMP_LOG_D(logger_, "Volume change: {}.", vol);
    }
}

void AudioPlayer::onError(const std::exception& e) {
    playback_state_.is_playing = false;
    if (const auto adapter = state_adapter_.lock()) {
        adapter->onError(e);
    }
}

void AudioPlayer::onDeviceStateChange(DeviceState state, const std::string & device_id) {    
    if (const auto state_adapter = state_adapter_.lock()) {
        switch (state) {
        case DeviceState::DEVICE_STATE_ADDED:
            XAMP_LOG_D(logger_, "Device added device id:{}.", device_id);
            state_adapter->onDeviceChanged(
                DeviceState::DEVICE_STATE_ADDED,
                device_id);
            break;
        case DeviceState::DEVICE_STATE_REMOVED:
            XAMP_LOG_D(logger_, "Device removed device id:{}.", device_id);
            if (device_id == device_id_) {
                // TODO: In many system has more ASIO device.
                if (IsAsioDevice(device_type_->getTypeId())) {
                    ResetAsioDriver();
                }
                
                state_adapter->onDeviceChanged(
                    DeviceState::DEVICE_STATE_REMOVED,
                    device_id);
                if (device_ != nullptr) {
                    device_->abortStream();
                    XAMP_LOG_D(logger_, "Device abort stream id:{}.", device_id);
                }
            }
            break;
        case DeviceState::DEVICE_STATE_DEFAULT_DEVICE_CHANGE:
            XAMP_LOG_D(logger_, "Default device device id:{}.", device_id);
            state_adapter->onDeviceChanged(
                DeviceState::DEVICE_STATE_DEFAULT_DEVICE_CHANGE,
                device_id);
            break;
        }
    }
}

void AudioPlayer::onGlitch(std::chrono::milliseconds duration, uint32_t count) {
    if (duration.count() == 0) {
        return;
    }
    XAMP_LOG_DEBUG("Audio glitch duration:{} ms, count:{}.", 
		duration.count(), count);
}

void AudioPlayer::openDevice(double stream_time) {
#if defined(XAMP_OS_WIN)
    if (auto* dsd_output = AsDsdDevice(device_)) {
        if (audio_config_.dsd_mode == DsdModes::DSD_MODE_AUTO
            || audio_config_.dsd_mode == DsdModes::DSD_MODE_PCM
            || audio_config_.dsd_mode == DsdModes::DSD_MODE_DOP) {
            if (const auto* const dsd_stream = asDsdStream(file_stream_)) {
                if (audio_config_.dsd_mode == DsdModes::DSD_MODE_NATIVE) {
                    dsd_output->setIoFormat(DsdIoFormat::IO_FORMAT_DSD);
                }
                else {
                    if (audio_config_.dsd_mode == DsdModes::DSD_MODE_DOP) {
                        dsd_output->setIoFormat(DsdIoFormat::IO_FORMAT_DOP);
                    } else {
                        dsd_output->setIoFormat(DsdIoFormat::IO_FORMAT_PCM);
                    }
                }
            }            
        } else {
            output_format_.setFormat(DataFormat::FORMAT_DSD);
            output_format_.setByteFormat(ByteFormat::SINT8);
            dsd_output->setIoFormat(DsdIoFormat::IO_FORMAT_DSD);
        }
    }
#endif
    device_->openStream(output_format_);
    device_->setVolume(audio_config_.volume);
    device_->setMute(is_muted_);
    device_->setStreamTime(stream_time);
}

void AudioPlayer::bufferStream(double stream_time, 
    const std::optional<double>& offset,
    const std::optional<double>& duration) {
    XAMP_LOG_D(logger_, "Buffing samples : {:.2f}ms", stream_time);

	if (dsp_manager_->contains(XAMP_UUID_OF(R8brainSampleRateConverter))) {
        setReadSampleSize(kR8brainBufferSize);
	}

    if (offset.has_value()) {
        playback_state_.stream_offset_time = offset.value();
    }

    if (duration.has_value()) {
        playback_state_.stream_duration = duration.value();
    }

    fifo_.clear();
    file_stream_->seek(playback_state_.stream_offset_time + stream_time);
    audio_config_.sample_size = file_stream_->getSampleSize();
    bufferSamples(file_stream_, GetBufferCount(output_format_.getSampleRate()) * 2);
}

void AudioPlayer::updatePlayerStreamTime(uint32_t stream_time_sec_unit) {
    playback_state_.stream_time_sec_unit.exchange(stream_time_sec_unit);
}

void AudioPlayer::setStateAdapter(const std::weak_ptr<IPlaybackStateAdapter>& adapter) {
    state_adapter_ = adapter;
    device_manager_->registerDeviceListener(shared_from_this());

    std::weak_ptr<AudioPlayer> player = shared_from_this();
    timer_.start(kUpdateSampleIntervalMs, [player]() {
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
                p->setState(PlayerState::PLAYER_STATE_STOPPED);
                p->playback_state_.is_playing = false;
            }
        }
        });
}

void AudioPlayer::seek(double stream_time) {
    if (!device_ || !device_->isStreamOpen()) {
        return;
    }

    bool expected = false;
    if (!playback_state_.is_seeking.compare_exchange_strong(expected, true)) {
        return;
    }
    XAMP_ON_SCOPE_EXIT(playback_state_.is_seeking = false);

    try {
        read_finish_and_wait_seek_signal_cond_.notify_all();
        pause();

        std::unique_lock<FastMutex> stream_lock{ stream_mutex_ };
        if (!file_stream_) {
            XAMP_LOG_D(logger_, "seek skipped because file stream is closed.");
            resume();
            return;
        }
        doSeek(stream_time);
    }
    catch (const std::exception& e) {
        XAMP_LOG_D(logger_, "seek failed: {}.", e.what());
        resume();
    }
    catch (...) {
        XAMP_LOG_D(logger_, "seek failed.");
        resume();
    }
}

void AudioPlayer::setParametricEq(bool enabled, const EqSettings& settings) {
    if (enabled) {
        config_.create(DspConfig::kEQSettings, settings);
    }
    else {
        config_.remove(DspConfig::kEQSettings);
    }

    std::lock_guard<FastMutex> stream_lock{ stream_mutex_ };
    if (!device_ || !device_->isStreamOpen()) {
        if (enabled) {
            dsp_manager_->addParametricEq();
        }
        else {
            dsp_manager_->removeParametricEq();
        }
        return;
    }

    const auto was_running = device_->isStreamRunning();
    const auto stream_time = device_->getStreamTime();
    if (was_running) {
        device_->stopStream(false);
    }

    try {
        dsp_manager_->setParametricEq(enabled, settings, config_);
        bufferStream(stream_time);
        device_->setStreamTime(stream_time);
    }
    catch (...) {
        if (was_running && device_->isStreamOpen()) {
            device_->startStream();
        }
        throw;
    }

    if (was_running) {
        device_->startStream();
    }
}

void AudioPlayer::doSeek(double stream_time) {
	if (playback_state_.state != PlayerState::PLAYER_STATE_PAUSED) {
        pause();
	}
	
    try {
        file_stream_->seek(stream_time);
    }
    catch (const Exception& e) {
        XAMP_LOG_D(logger_, e.getErrorMessage());
        resume();
        return;
    }

    device_->setStreamTime(stream_time);
    sample_end_time_ = file_stream_->getDuration() - stream_time;
    XAMP_LOG_D(logger_, "Stream duration:{:.2f} seeking:{:.2f} sec, end time:{:.2f} sec.",
        file_stream_->getDuration(),
        stream_time,
        sample_end_time_.load());
    auto seek_time = static_cast<uint32_t>(stream_time * 1000.0);
    if (seek_time >playback_state_.stream_time_sec_unit) {
        seek_time = static_cast<uint32_t>(round(stream_time, 2) * 1000.0);
    }
    updatePlayerStreamTime(seek_time);
    fifo_.clear();
    bufferStream(stream_time);
    resume();
}

void AudioPlayer::bufferSamples(const ScopedPtr<FileStream>& stream,
    int32_t buffer_count) {
    auto* const sample_buffer = read_buffer_.get();

    for (auto i = 0; i < buffer_count && file_stream_->isActive(); ++i) {
        XAMP_LOG_D(logger_, "Buffering {} ...", i);

        while (true) {
            if (!hasEnoughFifoWriteSpace(num_read_buffer_size_)) {
                return;
            }

            const auto num_samples = stream->getSamples(sample_buffer, 
                num_read_buffer_size_);
            if (num_samples == 0) {
                return;
            }

            auto* samples = reinterpret_cast<const float*>(sample_buffer);
            if (dsp_manager_->processDSP(samples, num_samples, fifo_)) {
                continue;
            }            
            break;
        }
    }
}

const ScopedPtr<IAudioDeviceManager>& AudioPlayer::getAudioDeviceManager() {
    return device_manager_;
}

ScopedPtr<IDSPManager>& AudioPlayer::getDspManager() {
    return dsp_manager_;
}

void AudioPlayer::setReadSampleSize(uint32_t num_samples) {
    num_read_buffer_size_ = num_samples;
    XAMP_LOG_D(logger_,
        "Output buffer:{} device format: {} num_read_sample: {} fifo buffer: {}.",
        device_->getBufferSize(),
        output_format_.toString(),
        String::formatBytes(num_read_buffer_size_),
        String::formatBytes(fifo_.size()));
}

void AudioPlayer::waitForReadFinishAndSeekSignal(
    std::unique_lock<FastMutex>& stopped_lock) {
    if (read_finish_and_wait_seek_signal_cond_.wait_for(stopped_lock,
        kWaitForSignalWhenReadFinish) != std::cv_status::timeout) {
        XAMP_LOG_T(logger_, "Stream is read done!, Weak up for seek signal.");
    }
}

bool AudioPlayer::shouldKeepReading() const {
    return playback_state_.is_playing && !playback_state_.is_seeking && file_stream_->isActive();
}

uint32_t AudioPlayer::estimateDspOutputBytes(uint32_t input_samples) const {
    if (input_samples == 0) {
        return 0;
    }

    if (audio_config_.dsd_mode == DsdModes::DSD_MODE_NATIVE) {
        return input_samples;
    }

    const uint32_t sample_size = file_stream_ != nullptr
        ? file_stream_->getSampleSize()
        : sizeof(float);
    const uint32_t input_sample_rate = input_format_.getSampleRate();
    const uint32_t output_sample_rate = output_format_.getSampleRate();

    uint64_t output_samples = input_samples;
    if (input_sample_rate != 0 && output_sample_rate > input_sample_rate) {
        output_samples = (static_cast<uint64_t>(input_samples) * output_sample_rate
            + input_sample_rate - 1) / input_sample_rate;
        output_samples += kResamplerOutputPaddingSamples;
    }

    const uint64_t output_bytes = output_samples * sample_size;
    return static_cast<uint32_t>((std::min)(output_bytes,
        static_cast<uint64_t>(std::numeric_limits<uint32_t>::max())));
}

bool AudioPlayer::hasEnoughFifoWriteSpace(uint32_t input_samples) const {
    const auto estimated_write_size = (std::max)(estimateDspOutputBytes(input_samples),
        min_fifo_write_size_);
    const auto max_available_write = fifo_.size() > 0 ? fifo_.size() - 1 : 0;
    const auto required_write_size = (std::min)(static_cast<size_t>(estimated_write_size),
        max_available_write);
    return fifo_.getAvailableWrite() >= required_write_size;
}

void AudioPlayer::readSampleLoop(std::byte* buffer,
    uint32_t buffer_size, 
    std::unique_lock<FastMutex>& stopped_lock) {
    if (!file_stream_->isActive()) {
        if (playback_state_.is_playing) {
            waitForReadFinishAndSeekSignal(stopped_lock);
        }
        return;
    }

    std::lock_guard<FastMutex> stream_lock{ stream_mutex_ };
    if (!file_stream_->isActive()) {
        return;
    }

	auto* bass_stream = dynamic_cast<BassFileStream*>(file_stream_.get());

    while (shouldKeepReading()) {
        if (!hasEnoughFifoWriteSpace(buffer_size)) {
            break;
        }

        const auto num_samples = file_stream_->getSamples(buffer, buffer_size);

        if (num_samples > 0) {
            auto* samples = reinterpret_cast<float*>(buffer);
            
            if (dsp_manager_->processDSP(samples, num_samples, fifo_)) {
                continue;
            }
        }

        if (num_samples == 0) {
            if (bass_stream != nullptr && !bass_stream->endOfStream()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                XAMP_LOG_I(logger_, "Player waiting data ...");
                continue;
            }
        }

        if (!isAvailableWrite()) {
            break;
        }        
    }
}

bool AudioPlayer::isAvailableWrite() const {
    const auto write_watermark = num_write_buffer_size_ * kMaxWriteRatio;
    const auto required_write_size = (std::max)(write_watermark, min_fifo_write_size_);
    const auto max_available_write = fifo_.size() > 0 ? fifo_.size() - 1 : 0;
    return fifo_.getAvailableWrite() >= (std::min)(
        static_cast<size_t>(required_write_size),
        max_available_write);
}

void AudioPlayer::play() {
    XAMP_LOG_W(logger_, "Player is playing!");

    if (!device_) {
        return;
    }

    if (const auto state = state_adapter_.lock()) {
        state->outputFormatChanged(output_format_, device_->getBufferSize());
    }

    playback_state_.is_playing = true;

    auto start_stream = [this]() {
        if (device_->isStreamOpen() && !device_->isStreamRunning()) {
            XAMP_LOG_D(logger_, "play volume:{} muted:{}.",
                audio_config_.volume, is_muted_);
            device_->startStream();
            setState(PlayerState::PLAYER_STATE_RUNNING);
        }
    };

    if (stream_task_.valid()) {
        XAMP_LOG_W(logger_, "Stream task is valid!");
        start_stream();
        return;
    }

    XAMP_LOG_W(logger_, "Stream task is spawning!");
    auto stream_task_started = std::make_shared<std::promise<void>>();
    auto stream_task_started_future = stream_task_started->get_future();

    stream_task_ = player_thread_pool_->spawn(
        SubmitPolicy::SUBMIT_POLICY_NORMAL,
        ExecuteFlags::EXECUTE_LONG_RUNNING,
        [player = shared_from_this(), stream_task_started](const auto& stop_token) {
        XAMP_LOG_W(player->logger_, "Stream task is spawn done!");
        stream_task_started->set_value();

        auto* p = player.get();

        std::unique_lock<FastMutex> pause_lock{ p->pause_mutex_ };
        std::unique_lock<FastMutex> stopped_lock{ p->stopped_mutex_ };

        auto* buffer = p->read_buffer_.get();
        const auto num_read_buffer_size = p->num_read_buffer_size_;
        const auto num_write_buffer_size = p->num_write_buffer_size_ * kMaxWriteRatio;

        XAMP_LOG_DEBUG("num_read_buffer_size: {}, num_write_buffer_size: {}",
            String::formatBytes(num_read_buffer_size),
            String::formatBytes(num_write_buffer_size)
        );

        WaitableTimer wait_timer;
        wait_timer.setTimeout(kReadSampleWaitTimeMs);

        try {
            while (p->playback_state_.is_playing && !stop_token.stop_requested()) {
                // Wait for pause signal.
                while (p->playback_state_.is_paused) {
                    p->pause_cond_.wait_for(pause_lock, kPauseWaitTimeout);
                }

                if (p->playback_state_.is_seeking) {
                    wait_timer.wait();
                    continue;
                }

                // Check stream is active.
                if (!p->isAvailableWrite()) {
                    // Wait for next available write time.
                    wait_timer.wait();
                    XAMP_LOG_T(p->logger_, "FIFO buffer: {} num_sample_write: {}",
                        p->fifo_.getAvailableWrite(),
                        num_write_buffer_size
                    );
                    continue;
                }
                p->readSampleLoop(buffer, num_read_buffer_size, stopped_lock);
            }
        }
        catch (const std::exception& e) {
            XAMP_LOG_D(p->logger_, "Stream thread read has exception: {}.", e.what());
            p->onError(e);
        }
        catch (...) {
            p->onError(std::exception());
        }

        XAMP_LOG_D(p->logger_, "Stream thread done!");
        {
            std::lock_guard<FastMutex> stream_lock{ p->stream_mutex_ };
            p->file_stream_.reset();
        }
    });

    if (stream_task_started_future.wait_for(kReadSampleWaitTimeMs) == std::future_status::timeout) {
        XAMP_LOG_W(logger_, "Stream task start wait timeout.");
    }

    start_stream();
}

void AudioPlayer::copySamples(void* samples, size_t num_samples) const {
    const auto adapter = state_adapter_.lock();
    if (!adapter) {
        return;
    }

    auto stream_time_sec_unit = playback_state_.stream_time_sec_unit.load();
    adapter->onSampleTime(stream_time_sec_unit / 1000.0);

    if (!isPcmAudio(audio_config_.dsd_mode)) {
        return;
    }

    Stopwatch watch;    
    watch.reset();    
    
    adapter->onSamplesChanged(static_cast<const float*>(samples), num_samples);
    auto elapsed = watch.elapsed<std::chrono::milliseconds>();
    if (elapsed >= kMinimalCopySamplesTime) {
        XAMP_LOG_W(logger_, "copySamples too slow ({} ms)!", elapsed.count());
    }
}

DataCallbackResult AudioPlayer::onGetSamples(void* samples,
    size_t num_buffer_frames, 
    size_t & num_filled_frames, 
    double stream_time, 
    double /*sample_time*/) {
    // sample_time is the device playback clock and may be reset to zero after stop.
    // stream_time is accumulated from rendered sample frames.
    const auto num_samples = num_buffer_frames * output_format_.getChannels();
    const auto sample_size = num_samples * audio_config_.sample_size;

    if (stream_time >= playback_state_.stream_duration) {
        updatePlayerStreamTime(kStopStreamTime);
        return DataCallbackResult::STOP;
    }

    size_t num_filled_bytes = 0;
    if (fifo_.tryRead(static_cast<std::byte*>(samples), sample_size, num_filled_bytes)) {
        num_filled_frames = num_filled_bytes
    	    / audio_config_.sample_size
    	    / output_format_.getChannels();
        if (num_filled_frames != num_buffer_frames) {            
            return DataCallbackResult::STOP;
        }
        updatePlayerStreamTime(static_cast<int32_t>(stream_time * 1000));
        copySamples(samples, num_samples);
        return DataCallbackResult::CONTINUE;
    }

    // Avoid stopping too early when the remaining audio is smaller than one render buffer.
    // stop on the next render callback after the logical stream end is reached.
    // 
    // (WASAPI render frame)       (WASAPI render end frame)
    //       |               |                 |
    //       |       (End audio time)          |
    //       V               V                 V
    // <--------------------------------------->
    //
    if (stream_time >= playback_state_.stream_duration) {
        updatePlayerStreamTime(kStopStreamTime);
        return DataCallbackResult::STOP;
    }

    num_filled_frames = num_buffer_frames;
    if (playback_state_.is_seeking) {
        updatePlayerStreamTime(static_cast<int32_t>(stream_time * 1000));
    }
    return DataCallbackResult::CONTINUE;
}

void AudioPlayer::prepareToPlay(ByteFormat byte_format,
    uint32_t device_sample_rate) {
    if (device_sample_rate != 0) {
        audio_config_.target_sample_rate = device_sample_rate;
    }

    setDeviceFormat();

	if (byte_format != ByteFormat::INVALID_FORMAT) {
        output_format_.setByteFormat(byte_format);
    }

    createDevice(device_info_.value().device_type_id,
        device_info_.value().device_id,
        false);
    openDevice(0);
    createBuffer();

    config_.create(DspConfig::kInputFormat,
        std::any(input_format_));
    config_.create(DspConfig::kOutputFormat,
        std::any(output_format_));
    config_.create(DspConfig::kDsdMode,
        std::any(audio_config_.dsd_mode));
    config_.create(DspConfig::kSampleSize,
        std::any(file_stream_->getSampleSize()));

    dsp_manager_->initialize(config_);
	sample_end_time_ = file_stream_->getDuration();
    XAMP_LOG_D(logger_, "Stream end time: {:.2f} sec.", sample_end_time_.load());    
}

Property& AudioPlayer::getDspConfig() {
    return config_;
}

uint32_t AudioPlayer::getBitRate() const {    
    return file_stream_->getBitRate();
}

XAMP_AUDIO_PLAYER_NAMESPACE_END
