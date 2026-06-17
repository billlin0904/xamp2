//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <player/playstate.h>
#include <player/iaudioplayer.h>

#include <base/base.h>
#include <base/audiobuffer.h>
#include <base/audioformat.h>
#include <base/timer.h>
#include <base/dsdsampleformat.h>
#include <base/memory.h>
#include <base/uuid.h>
#include <base/buffer.h>
#include <base/fastmutex.h>
#include <base/fastconditionvariable.h>
#include <base/task.h>
#include <base/archivefile.h>

#include <output_device/iaudiocallback.h>
#include <output_device/idevicestatelistener.h>
#include <output_device/deviceinfo.h>

#include <future>
#include <optional>
#include <any>

XAMP_AUDIO_PLAYER_NAMESPACE_BEGIN

struct PlaybackState {
	std::atomic<bool> is_seeking = false;
    std::atomic<bool> is_playing = false;
    std::atomic<bool> is_paused = false;
    std::atomic<PlayerState> state = PlayerState::PLAYER_STATE_STOPPED;
    std::atomic<uint32_t> stream_time_sec_unit = 0;
    double stream_offset_time = 0.0;
    double stream_duration = 0.0;
};

struct AudioConfig {
    uint32_t sample_size = 0;
    uint32_t target_sample_rate = 0;
    uint32_t volume = 0;
    DsdModes dsd_mode = DsdModes::DSD_MODE_PCM;
};

/*
* AudioPlayer is a class that plays audio files. It is responsible for reading audio files,
* 
*/
class AudioPlayer final :
    public IAudioCallback,
    public IDeviceStateListener,
    public IAudioPlayer,
    public std::enable_shared_from_this<AudioPlayer> {
public:
	static constexpr auto kStopStreamTime = std::numeric_limits<uint32_t>::max();
    
    AudioPlayer(const std::shared_ptr<IThreadPool>& playback_thread_pool,
        const std::shared_ptr<IThreadPool>& player_thread_pool);

    virtual ~AudioPlayer() override;

    XAMP_DISABLE_COPY(AudioPlayer)

    void openArchiveEntry(ArchiveEntry archive_entry,
        const DeviceInfo& device_info,
        uint32_t target_sample_rate = 0,
        DsdModes output_mode = DsdModes::DSD_MODE_AUTO) override;

    void open(ScopedPtr<FileStream> file_stream,
        const DeviceInfo& device_info,
        uint32_t target_sample_rate = 0,
        DsdModes output_mode = DsdModes::DSD_MODE_AUTO) override;

    void destroy() override;

    void setStateAdapter(const std::weak_ptr<IPlaybackStateAdapter>& adapter) override;

    void prepareToPlay(ByteFormat byte_format = ByteFormat::INVALID_FORMAT, uint32_t device_sample_rate = 0) override;

    void play() override;

    void pause() override;

    void resume() override;

    void stop(bool signal_to_stop = true, bool shutdown_device = false, bool wait_for_stop_stream = true) override;    
    	
    void seek(double stream_time) override;

    void setParametricEq(bool enabled, const EqSettings& settings) override;

    void setVolume(uint32_t volume) override;

    uint32_t getVolume() const override;

    bool isHardwareControlVolume() const override;

    bool isMute() const override;

    void setMute(bool mute) override;
    
    bool isPlaying() const override;

    DsdModes getDsdModes() const override;

    bool isDsdFile() const override;

    std::optional<uint32_t> getDsdSpeed() const override;

    double getDuration() const override;

    PlayerState getState() const override;

    AudioFormat getInputFormat() const override;

    AudioFormat getOutputFormat() const override;

    const ScopedPtr<IAudioDeviceManager>& getAudioDeviceManager() override;

    ScopedPtr<IDSPManager>& getDspManager() override;    

    void bufferStream(double stream_time = 0.0, const std::optional<double> & offset = std::nullopt, const std::optional<double>& duration = std::nullopt) override;

    Property& getDspConfig() override;

	uint32_t getBitRate() const override;
private:
    DataCallbackResult onGetSamples(void* samples,
        size_t num_buffer_frames, 
        size_t& num_filled_frames,
        double stream_time, 
        double sample_time) override;

    void onVolumeChange(int32_t vol) override;

    void onError(const std::exception& e) override;

    void onDeviceStateChange(DeviceState state, std::string const& device_id) override;

    void onGlitch(std::chrono::milliseconds duration, uint32_t count) override;

    void doSeek(double stream_time);
    	
    void openStream(ArchiveEntry archive_entry, DsdModes dsd_mode);

    void openStream(ScopedPtr<FileStream> file_stream, DsdModes dsd_mode);

    void createDevice(Uuid const& device_type_id, const  std::string & device_id, bool open_always);

    void closeDevice(bool wait_for_stop_stream, bool quit = false);

    void createBuffer();

    void setDeviceFormat();

    void openDevice(double stream_time = 0.0);

    void setState(PlayerState play_state);

    void readSampleLoop(std::byte* buffer, uint32_t buffer_size, std::unique_lock<FastMutex>& stopped_lock);

    void copySamples(void* samples, size_t num_samples) const;

    void bufferSamples(const ScopedPtr<FileStream>& stream, int32_t buffer_count = 1);

    void updatePlayerStreamTime(uint32_t stream_time_sec_unit = 0) ;

    void resizeReadBuffer(uint32_t allocate_size);

    void resizeFIFO(uint32_t fifo_size);

    void readStreamInfo(DsdModes dsd_mode, const ScopedPtr<FileStream>& stream);

    void waitForReadFinishAndSeekSignal(std::unique_lock<FastMutex>& stopped_lock);

    bool shouldKeepReading() const ;

    void setReadSampleSize(uint32_t num_samples);

    uint32_t estimateDspOutputBytes(uint32_t input_samples) const;

    bool hasEnoughFifoWriteSpace(uint32_t input_samples) const;

    bool isAvailableWrite() const ;

    bool is_muted_;
    bool is_dsd_file_;
    uint32_t num_read_buffer_size_;
    uint32_t num_write_buffer_size_;
    uint32_t min_fifo_write_size_;
    std::optional<uint32_t> dsd_speed_;
    std::atomic<double> sample_end_time_;
    AudioConfig audio_config_;
    PlaybackState playback_state_;
    mutable FastMutex pause_mutex_;
    mutable FastMutex stopped_mutex_;
    mutable FastMutex stream_mutex_;
    Uuid device_type_id_;
    Timer timer_;    
    AudioFormat input_format_;
    AudioFormat output_format_;
    ScopedPtr<FileStream> file_stream_;
    ScopedPtr<IDeviceType> device_type_;
    ScopedPtr<IOutputDevice> device_;
    ScopedPtr<IDSPManager> dsp_manager_;
    ScopedPtr<IAudioDeviceManager> device_manager_;
    std::weak_ptr<IPlaybackStateAdapter> state_adapter_;    
    Future<void> stream_task_;
    Property config_;
    LoggerPtr logger_;
    std::string device_id_;
    Buffer<std::byte> read_buffer_;
    std::optional<DeviceInfo> device_info_;
    FastConditionVariable pause_cond_;
    FastConditionVariable read_finish_and_wait_seek_signal_cond_;
    AudioBuffer<std::byte> fifo_;
    std::shared_ptr<IThreadPool> playback_thread_pool_;
	std::shared_ptr<IThreadPool> player_thread_pool_;
};

XAMP_AUDIO_PLAYER_NAMESPACE_END
