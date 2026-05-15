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
    
    AudioPlayer(const std::shared_ptr<IThreadPoolExecutor>& playback_thread_pool,
        const std::shared_ptr<IThreadPoolExecutor>& player_thread_pool);

    virtual ~AudioPlayer() override;

    XAMP_DISABLE_COPY(AudioPlayer)

    void Open(const Path& file_path,
        const Uuid& device_id = Uuid::kNullUuid,
        float rate = 0.0f,
        bool use_mqa_decode = false) override;

    void Open(const Path& file_path,              
        const DeviceInfo& device_info,
        uint32_t target_sample_rate = 0,
        DsdModes output_mode = DsdModes::DSD_MODE_AUTO,
        float rate = 0.0f,
        bool use_mqa_decode = false) override;

    void OpenArchiveEntry(ArchiveEntry archive_entry,
        const DeviceInfo& device_info,
        uint32_t target_sample_rate = 0,
        DsdModes output_mode = DsdModes::DSD_MODE_AUTO,
        float rate = 0.0f,
        bool use_mqa_decode = false) override;

    void Open(ScopedPtr<FileStream> file_stream,
        const DeviceInfo& device_info,
        uint32_t target_sample_rate = 0,
        DsdModes output_mode = DsdModes::DSD_MODE_AUTO) override;

    void Destroy() override;

    void SetStateAdapter(const std::weak_ptr<IPlaybackStateAdapter>& adapter) override;

    void PrepareToPlay(ByteFormat byte_format = ByteFormat::INVALID_FORMAT, uint32_t device_sample_rate = 0) override;

    void Play() override;

    void Pause() override;

    void Resume() override;

    void Stop(bool signal_to_stop = true, bool shutdown_device = false, bool wait_for_stop_stream = true) override;    
    	
    void Seek(double stream_time) override;

    void SetVolume(uint32_t volume) override;

    uint32_t GetVolume() const override;

    bool IsHardwareControlVolume() const override;

    bool IsMute() const override;

    void SetMute(bool mute) override;
    
    bool IsPlaying() const override;

    DsdModes GetDsdModes() const override;

    bool IsDsdFile() const override;

    std::optional<uint32_t> GetDsdSpeed() const override;

    double GetDuration() const override;

    PlayerState GetState() const override;

    AudioFormat GetInputFormat() const override;

    AudioFormat GetOutputFormat() const override;

    const ScopedPtr<IAudioDeviceManager>& GetAudioDeviceManager() override;

    ScopedPtr<IDSPManager>& GetDspManager() override;    

    void BufferStream(double stream_time = 0.0, const std::optional<double> & offset = std::nullopt, const std::optional<double>& duration = std::nullopt) override;

    void EnableFadeOut(bool enable) override;

    Property& GetDspConfig() override;

    void SetDelayCallback(std::function<void(uint32_t)> &&delay_callback) override;

    void SeFileCacheMode(bool enable) override;

	uint32_t GetBitRate() const override;

	bool IsMQA() const override;
private:
    DataCallbackResult OnGetSamples(void* samples,
        size_t num_buffer_frames, 
        size_t& num_filled_frames,
        double stream_time, 
        double sample_time) override;

    void OnVolumeChange(int32_t vol) override;

    void OnError(const std::exception& e) override;

    void OnDeviceStateChange(DeviceState state, std::string const& device_id) override;

    void OnGlitch(std::chrono::milliseconds duration, uint32_t count) override;

    void DoSeek(double stream_time);
    	
    void OpenStream(Path const& file_path, DsdModes dsd_mode, float rate, bool use_mqa_decode);

    void OpenStream(ArchiveEntry archive_entry, DsdModes dsd_mode, float rate, bool use_mqa_decode);

    void OpenStream(ScopedPtr<FileStream> file_stream, DsdModes dsd_mode);

    void CreateDevice(Uuid const& device_type_id, const  std::string & device_id, bool open_always);

    void CloseDevice(bool wait_for_stop_stream, bool quit = false);

    void CreateBuffer();

    void SetDeviceFormat();

    void OpenDevice(double stream_time = 0.0);

    void SetState(PlayerState play_state);

    void ReadSampleLoop(std::byte* buffer, uint32_t buffer_size, std::unique_lock<FastMutex>& stopped_lock);

    void CopySamples(void * samples, size_t num_buffer_frames) const;

    void BufferSamples(const ScopedPtr<FileStream>& stream, int32_t buffer_count = 1);

    void UpdatePlayerStreamTime(uint32_t stream_time_sec_unit = 0) ;

    void ResizeReadBuffer(uint32_t allocate_size);

    void ResizeFIFO(uint32_t fifo_size);

    void ReadStreamInfo(DsdModes dsd_mode, const ScopedPtr<FileStream>& stream);

    void WaitForReadFinishAndSeekSignal(std::unique_lock<FastMutex>& stopped_lock);

    bool ShouldKeepReading() const ;

    void SetReadSampleSize(uint32_t num_samples);

    bool IsAvailableWrite() const ;

    bool is_muted_;
    bool is_dsd_file_;
    bool enable_file_cache_;
    uint32_t num_read_buffer_size_;
    uint32_t num_write_buffer_size_;
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
    std::function<void(uint32_t)> delay_callback_;
    std::optional<DeviceInfo> device_info_;
    FastConditionVariable pause_cond_;
    FastConditionVariable read_finish_and_wait_seek_signal_cond_;
    AudioBuffer<std::byte> fifo_;
    std::shared_ptr<IThreadPoolExecutor> playback_thread_pool_;
	std::shared_ptr<IThreadPoolExecutor> player_thread_pool_;
};

XAMP_AUDIO_PLAYER_NAMESPACE_END
