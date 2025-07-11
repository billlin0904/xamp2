//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
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
#include <base/workstealingtaskqueue.h>
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
#include <variant>

XAMP_AUDIO_PLAYER_NAMESPACE_BEGIN

XAMP_MAKE_ENUM(PlayerActionId,
    PLAYER_SEEK);

struct SeekAction {
    double stream_time;
};

using PlayerActionContent = std::variant<SeekAction>;

/*
* PlayerAction is a struct that contains the player action.
* 
* @see PlayerActionId
*/
struct PlayerAction {
    PlayerActionId id;
    PlayerActionContent content;
};

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
    static constexpr auto kFadeTimeSeconds = 1;
    
    /*
    * Constructor.
    */
    AudioPlayer(const std::shared_ptr<IThreadPoolExecutor>& playback_thread_pool, const std::shared_ptr<IThreadPoolExecutor>& player_thread_pool);

    /*
    * Destructor.
    */
    virtual ~AudioPlayer() override;

    XAMP_DISABLE_COPY(AudioPlayer)

    /*
    * Destroy.
    *  
    */
    void Destroy() override;

    /*
    * Startup the player.
    * 
    * @param[in] adapter The playback state adapter.
    */
	void SetStateAdapter(const std::weak_ptr<IPlaybackStateAdapter>& adapter) override;

    /*
    * Open a file.
    * 
    * @param[in] file_path The file path.
    * @param[in] device_id The device id.
    */
    void Open(const Path& file_path, const Uuid& device_id = Uuid::kNullUuid) override;

    /*
    * Open a file.
    * 
    * @param[in] file_path The file path.
    * @param[in] device_info The device info.
    * @param[in] target_sample_rate The target sample rate.
    */
    void Open(const Path& file_path,              
        const DeviceInfo& device_info,
        uint32_t target_sample_rate = 0,
        DsdModes output_mode = DsdModes::DSD_MODE_AUTO) override;

    void OpenArchiveEntry(ArchiveEntry archive_entry,
        const DeviceInfo& device_info,
        uint32_t target_sample_rate = 0,
        DsdModes output_mode = DsdModes::DSD_MODE_AUTO) override;

    /*
    * Prepare to play.
    * 
    * @param[in] byte_format The byte format.
    * @param[in] device_sample_rate The device sample rate.
    * @param[in] dsd_mode The dsd mode.
    * @param[in] dsd_speed The dsd speed.
    */
    void PrepareToPlay(ByteFormat byte_format = ByteFormat::INVALID_FORMAT, uint32_t device_sample_rate = 0) override;

    /*
    * Play.
    *     
    */
    void Play() override;

    /*
    * Pause.
    * 
    */
    void Pause() override;

    /*
    * Resume.
    * 
    */
    void Resume() override;

    /*
    * Stop.
    * 
    * @param[in] signal_to_stop The signal to stop.
    * @param[in] shutdown_device The shutdown device.
    * @param[in] wait_for_stop_stream The wait for stop stream.
    */
    void Stop(bool signal_to_stop = true, bool shutdown_device = false, bool wait_for_stop_stream = true) override;    
    	
    /*
    * Seek.
    * 
    * @param[in] stream_time The stream time.
    */
    void Seek(double stream_time) override;

    /*
    * Set volume.
    * 
    * @param[in] volume The volume.
    */
    void SetVolume(uint32_t volume) override;

    /*
    * Get volume.
    * 
    * @return The volume.
    */
    uint32_t GetVolume() const override;

    /*
    * Is hardware control volume.
    * 
    * @return True if hardware control volume.
    */
    bool IsHardwareControlVolume() const override;

    /*
    * Is mute.
    * 
    * @return True if mute.
    */
    bool IsMute() const override;

    /*
    * Set mute.
    * 
    * @param[in] mute The mute.
    */
    void SetMute(bool mute) override;
    
    /*
    * Is playing.
    * 
    * @return True if playing.
    */
    bool IsPlaying() const noexcept override;

    /*
    * Get dsd modes.
    * 
    * @return The dsd modes.
    */
    DsdModes GetDsdModes() const noexcept override;

    /*
    * Is dsd file.
    * 
    * @return True if dsd file.
    */
    bool IsDsdFile() const override;

    /*
    * Get dsd speed.
    * 
    * @return The dsd speed.
    */
    std::optional<uint32_t> GetDsdSpeed() const override;

    /*
    * Get media duration.
    * 
    * @return The media duration.
    */
    double GetDuration() const override;

    /*
    * Get player state.
    * 
    * @return The player state.
    */
    PlayerState GetState() const noexcept override;

    /*
    * Get input format.
    * 
    * @return The input format.
    */
    AudioFormat GetInputFormat() const noexcept override;

    /*
    * Get output format.
    * 
    * @return The output format.
    */
    AudioFormat GetOutputFormat() const noexcept override;

    /*
    * Get audio device manager.
    * 
    * @return The audio device manager.
    */
    const ScopedPtr<IAudioDeviceManager>& GetAudioDeviceManager() override;

    /*
    * Get dsp manager.
    * 
    * @return The dsp manager.
    */
    ScopedPtr<IDSPManager>& GetDspManager() override;    

    /*
    * Buffer stream.
    * 
    * @param[in] stream_time The stream time.
    */
    void BufferStream(double stream_time = 0.0, const std::optional<double> & offset = std::nullopt, const std::optional<double>& duration = std::nullopt) override;

    /*
    * Enable fade out.
    * 
    * @param[in] enable The enable.
    */
    void EnableFadeOut(bool enable) override;

    /*
    * Get dsp config.
    * 
    * @return The dsp config.
    */
    AnyMap& GetDspConfig() override;

    /*
     * Set audio delay callback.
     */
    void SetDelayCallback(std::function<void(uint32_t)> &&delay_callback) override;

   /*
    * Set file cache mode.
    */
    void SeFileCacheMode(bool enable) override;
private:
    DataCallbackResult OnGetSamples(void* samples,
        size_t num_buffer_frames, 
        size_t& num_filled_frames,
        double stream_time, 
        double sample_time) noexcept override;

    void OnVolumeChange(int32_t vol) noexcept override;

    void OnError(const std::exception& e) noexcept override;

    void OnDeviceStateChange(DeviceState state, std::string const& device_id) override;

    void DoSeek(double stream_time);        
    	
    void OpenStream(Path const& file_path, DsdModes dsd_mode);

    void OpenStream(ArchiveEntry archive_entry, DsdModes dsd_mode);

    void CreateDevice(Uuid const& device_type_id, const  std::string & device_id, bool open_always);

    void CloseDevice(bool wait_for_stop_stream, bool quit = false);

    void CreateBuffer();

    void SetDeviceFormat();

    void OpenDevice(double stream_time = 0.0);

    void SetState(PlayerState play_state);

    void ReadSampleLoop(std::byte* buffer, uint32_t buffer_size, std::unique_lock<FastMutex>& stopped_lock);

    void CopySamples(void * samples, size_t num_buffer_frames) const;

    void BufferSamples(const ScopedPtr<FileStream>& stream, int32_t buffer_count = 1);

    void UpdatePlayerStreamTime(uint32_t stream_time_sec_unit = 0) noexcept;

    void ResizeReadBuffer(uint32_t allocate_size);

    void ResizeFIFO(uint32_t fifo_size);

    void ReadPlayerAction();

    void ReadStreamInfo(DsdModes dsd_mode, const ScopedPtr<FileStream>& stream);

    void ProcessFadeOut();

    void FadeOut();

    void WaitForReadFinishAndSeekSignal(std::unique_lock<FastMutex>& stopped_lock);

    bool ShouldKeepReading() const noexcept;

    void SetReadSampleSize(uint32_t num_samples);

    bool IsAvailableWrite() const noexcept;

    bool is_muted_;
    bool is_dsd_file_;
    bool enable_fadeout_;
    bool enable_file_cache_;
    uint32_t num_read_buffer_size_;
    uint32_t num_write_buffer_size_;
    std::optional<uint32_t> dsd_speed_;
    std::atomic<bool> is_fade_out_;
    std::atomic<double> sample_end_time_;
    AudioConfig audio_config_;
	PlaybackState playback_state_;
    mutable FastMutex pause_mutex_;
    mutable FastMutex stopped_mutex_;    
    Uuid device_type_id_;
    Timer timer_;    
    AudioFormat input_format_;
    AudioFormat output_format_;
    ScopedPtr<FileStream> stream_;
    ScopedPtr<IDeviceType> device_type_;
    ScopedPtr<IOutputDevice> device_;
    ScopedPtr<IDSPManager> dsp_manager_;
    ScopedPtr<IAudioProcessor> fader_;
    ScopedPtr<IAudioDeviceManager> device_manager_;
    std::weak_ptr<IPlaybackStateAdapter> state_adapter_;    
    Task<void> stream_task_;        
    AnyMap config_;
    LoggerPtr logger_;
    std::string device_id_;
    Buffer<std::byte> read_buffer_;
    std::function<void(uint32_t)> delay_callback_;
    std::optional<DeviceInfo> device_info_;
    FastConditionVariable pause_cond_;
    FastConditionVariable read_finish_and_wait_seek_signal_cond_;
    ConcurrentQueue<PlayerAction> action_queue_;
    AudioBuffer<std::byte> fifo_;
    std::shared_ptr<IThreadPoolExecutor> playback_thread_pool_;
	std::shared_ptr<IThreadPoolExecutor> player_thread_pool_;
};

XAMP_AUDIO_PLAYER_NAMESPACE_END
