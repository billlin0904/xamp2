//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <future>
#include <optional>
#include <any>

#include <base/base.h>
#include <base/audiobuffer.h>
#include <base/audioformat.h>
#include <base/timer.h>
#include <base/dsdsampleformat.h>
#include <base/align_ptr.h>
#include <base/uuid.h>
#include <base/spsc_queue.h>
#include <base/buffer.h>
#include <base/fastmutex.h>
#include <base/fastconditionvariable.h>
#include <base/ithreadpool.h>

#include <stream/stream.h>

#include <output_device/iaudiocallback.h>
#include <output_device/idevicestatelistener.h>
#include <output_device/deviceinfo.h>
#include <player/playstate.h>
#include <player/iaudioplayer.h>

namespace xamp::player {

MAKE_XAMP_ENUM(PlayerActionId,
    PLAYER_SEEK);

struct PlayerAction {
    PlayerActionId id;
    std::any content;
};

class AudioPlayer final :
    public IAudioCallback,
    public IDeviceStateListener,
    public IAudioPlayer,
    public std::enable_shared_from_this<AudioPlayer> {
public:
    AudioPlayer();

    virtual ~AudioPlayer() override;

    explicit AudioPlayer(const std::weak_ptr<IPlaybackStateAdapter>& adapter);

    XAMP_DISABLE_COPY(AudioPlayer)

	void Startup() override;

    void Open(Path const& file_path, const Uuid& device_id = Uuid::kNullUuid) override;

    void Open(Path const& file_path, const DeviceInfo& device_info, uint32_t target_sample_rate = 0) override;

    void PrepareToPlay() override;

    void Play() override;

    void Pause() override;

    void Resume() override;

    void Stop(bool signal_to_stop = true, bool shutdown_device = false, bool wait_for_stop_stream = true) override;

    void Destroy() override;
    	
    void Seek(double stream_time) override;

    void SetVolume(uint32_t volume) override;

    uint32_t GetVolume() const override;

    bool IsHardwareControlVolume() const override;

    bool IsMute() const override;

    void SetMute(bool mute) override;

    bool IsPlaying() const noexcept override;

    DsdModes GetDsdModes() const noexcept override;

    bool IsDSDFile() const override;

    std::optional<uint32_t> GetDSDSpeed() const override;

    double GetDuration() const override;

    double GetStreamTime() const override;

    PlayerState GetState() const noexcept override;

    AudioFormat GetInputFormat() const noexcept override;

    AudioFormat GetOutputFormat() const noexcept override;

    void SetDevice(const DeviceInfo& device_info) override;

    DeviceInfo GetDevice() const override;

    const AlignPtr<IAudioDeviceManager>& GetAudioDeviceManager() override;

    AlignPtr<IDSPManager>& GetDSPManager() override;

    bool CanConverter() const noexcept;

private:
    void DoSeek(double stream_time);        
    	
    void OpenStream(Path const& file_path, DeviceInfo const& device_info);

    void CreateDevice(Uuid const& device_type_id, std::string const & device_id, bool open_always);

    void CloseDevice(bool wait_for_stop_stream, bool quit = false);

    void CreateBuffer();

    void SetDeviceFormat();

    void BufferStream(double stream_time = 0.0);

    DataCallbackResult OnGetSamples(void* samples, size_t num_buffer_frames, size_t& num_filled_frames, double stream_time, double sample_time) noexcept override;

    void OnVolumeChange(float vol) noexcept override;

    void OnError(Exception const & e) noexcept override;

    void OnDeviceStateChange(DeviceState state, std::string const & device_id) override;

    void OpenDevice(double stream_time = 0.0);

    void SetState(PlayerState play_state);

    void ReadSampleLoop(int8_t* sample_buffer, uint32_t max_buffer_sample, std::unique_lock<FastMutex> &stopped_lock);

    void CopySamples(void * samples, size_t num_buffer_frames) const;

    void BufferSamples(const AlignPtr<IAudioStream>& stream, int32_t buffer_count = 1);

    void UpdateProgress(int32_t sample_size = 0) noexcept;

    void AllocateReadBuffer(uint32_t allocate_size);

    void AllocateFifo();

    void ProcessPlayerAction();

    void SetDSDStreamMode(DsdModes dsd_mode, AlignPtr<IAudioStream>& stream);

    void ProcessFadeOut();

    void FadeOut();

    bool is_muted_;
    bool is_dsd_file_;
    DsdModes dsd_mode_;
    uint8_t sample_size_;
    uint32_t target_sample_rate_;
    uint32_t fifo_size_;
    uint32_t num_read_sample_;
    uint32_t volume_;
    std::optional<uint32_t> dsd_speed_;
    std::atomic<bool> is_fade_out_;
    std::atomic<bool> is_playing_;
    std::atomic<bool> is_paused_;
    std::atomic<int32_t> playback_event_;
    std::atomic<PlayerState> state_;
    std::atomic<double> sample_end_time_;
    std::atomic<double> stream_duration_;
    mutable FastMutex pause_mutex_;
    mutable FastMutex stopped_mutex_;
    std::string device_id_;
    Uuid device_type_id_;
    Timer timer_;    
    FastConditionVariable pause_cond_;
    FastConditionVariable read_finish_and_wait_seek_signal_cond_;
    AudioFormat input_format_;
    AudioFormat output_format_;
    AlignPtr<IAudioDeviceManager> device_manager_;
    AlignPtr<IAudioStream> stream_;
    AlignPtr<IDeviceType> device_type_;
    AlignPtr<IOutputDevice> device_;
    std::weak_ptr<IPlaybackStateAdapter> state_adapter_;
    AudioBuffer<int8_t> fifo_;
    Buffer<int8_t> read_buffer_;
    DeviceInfo device_info_;    
    SharedFuture<void> stream_task_;
    SpscQueue<PlayerAction> action_queue_;
    AlignPtr<IDSPManager> dsp_manager_;
    AlignPtr<IAudioProcessor> fader_;
    std::shared_ptr<Logger> logger_;
};

}