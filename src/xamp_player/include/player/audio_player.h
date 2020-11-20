//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <cstdint>
#include <mutex>
#include <future>
#include <optional>

#include <base/base.h>
#include <base/audiobuffer.h>
#include <base/audioformat.h>
#include <base/timer.h>
#include <base/dsdsampleformat.h>
#include <base/align_ptr.h>
#include <base/uuid.h>
#include <base/spsc_queue.h>
#include <base/stopwatch.h>

#include <stream/stream.h>
#include <stream/equalizer.h>

#include <output_device/output_device.h>
#include <output_device/audiocallback.h>
#include <output_device/deviceinfo.h>

#include <player/playstate.h>
#include <player/playbackstateadapter.h>
#include <player/player.h>

namespace xamp::player {

using namespace xamp::base;
using namespace xamp::stream;
using namespace xamp::output_device;

class XAMP_PLAYER_API AudioPlayer final :
    public AudioCallback,
    public DeviceStateListener,
    public std::enable_shared_from_this<AudioPlayer> {
public:
    enum class GaplessPlayState {
        INIT,
        BUFFING,
        ABORT,
    };

    enum class GaplessPlayMsgID {
        SWITCH,
    };

    XAMP_DISABLE_COPY(AudioPlayer)

    AudioPlayer();

    virtual ~AudioPlayer() override;

    explicit AudioPlayer(std::weak_ptr<PlaybackStateAdapter> adapter);

    static void Initial();

    void Open(std::wstring const& file_path, std::wstring const& file_ext, const DeviceInfo& device_info, bool use_native_dsd);

    void StartPlay(double start_time = 0.0, double end_time = 0.0);

    void Play();

    void Pause();

    void Resume();

    void Stop(bool signal_to_stop = true, bool shutdown_device = false, bool wait_for_stop_stream = true);

    void Destroy();

    void SetLoop(double start_time, double end_time);

    void Seek(double stream_time);    

    void SetVolume(uint32_t volume);

    uint32_t GetVolume() const;

    bool IsHardwareControlVolume() const;

    bool IsMute() const;

    void SetMute(bool mute);

    bool IsPlaying() const noexcept;

    DsdModes GetDsdModes() const noexcept;

    bool IsDSDFile() const;

    std::optional<uint32_t> GetDSDSpeed() const;

    double GetDuration() const;

    PlayerState GetState() const noexcept;

    AudioFormat GetFileFormat() const noexcept;

    AudioFormat GetOutputFormat() const noexcept;

    bool IsDsdStream() const noexcept;

    void SetResampler(uint32_t samplerate, AlignPtr<Resampler> &&resampler);

    void EnableResampler(bool enable = true);

    bool IsEnableResampler() const;

    void EnableEQ(bool enable = true);

    void SetEQ(uint32_t band, float gain, float Q);

    void SetEQ(std::array<EQSettings, kMaxBand> const &bands);

    void SetDevice(const DeviceInfo& device_info);

    DeviceInfo GetDevice() const;

    bool IsGaplessPlay() const;

    void EnableGaplessPlay(bool enable);

    void ClearPlayQueue();    

private:
    void Startup();
    	
    void OpenStream(std::wstring const & file_path, std::wstring const & file_ext, DeviceInfo const& device_info, bool use_native_dsd);

    void CreateDevice(Uuid const& device_type_id, std::string const & device_id, bool open_always);

    void CloseDevice(bool wait_for_stop_stream);

    void CreateBuffer();

    void SetDeviceFormat();

    void BufferStream(double stream_time = 0.0);

    int32_t OnGetSamples(void* samples, uint32_t num_buffer_frames, double stream_time, double sample_time) noexcept override;

    void OnVolumeChange(float vol) noexcept override;

    void OnError(Exception const & e) noexcept override;

    void OnDeviceStateChange(DeviceState state, std::string const & device_id) override;

    void OpenDevice(double stream_time = 0.0);

    void SetState(PlayerState play_state);

    void ReadSampleLoop(int8_t* sample_buffer, uint32_t max_read_sample, std::unique_lock<std::mutex> &lock, AlignPtr<Resampler> &resampler);

    void BufferSamples(AlignPtr<FileStream>& stream, AlignPtr<Resampler> &resampler, int32_t buffer_count = 1);

    void UpdateSlice(float const *samples = nullptr, int32_t sample_size = 0, double stream_time = 0.0) noexcept;

    void OnGaplessPlayState(std::unique_lock<std::mutex>& lock, AlignPtr<Resampler> &resampler);

    struct XAMP_CACHE_ALIGNED(kMallocAlignSize) AudioSlice {
        AudioSlice(float const *samples = nullptr, int32_t sample_size = 0, double stream_time = 0.0) noexcept
            : samples(samples)
            , sample_size(sample_size)
            , stream_time(stream_time) {
        }
        float const *samples;
        int32_t sample_size;
        double stream_time;
    };

    XAMP_ENFORCE_TRIVIAL(AudioSlice)

    static constexpr auto kMsgQueueSize = 8;

    bool is_muted_;
    bool enable_resample_;
    DsdModes dsd_mode_;
    std::atomic<PlayerState> state_;
    uint8_t sample_size_;
    uint32_t target_samplerate_;
    uint32_t volume_;
    uint32_t num_buffer_samples_;
    uint32_t num_read_sample_;
    uint32_t read_sample_size_; 
    std::atomic<bool> is_playing_;
    std::atomic<bool> is_paused_;
    std::atomic<bool> enable_gapless_play_;
    std::atomic<double> sample_end_time_;
    std::atomic<double> stream_duration_;
    std::atomic<AudioSlice> slice_;
    mutable std::mutex pause_mutex_;
    mutable std::mutex stream_read_mutex_;
    GaplessPlayState gapless_play_state_ = GaplessPlayState::INIT;
#ifdef _DEBUG
    std::chrono::microseconds min_process_time_{ 0 };
    std::chrono::microseconds max_process_time_{ 0 };
#endif
    Stopwatch sw_;
    std::string device_id_;
    Uuid device_type_id_;
    std::condition_variable pause_cond_;
    std::condition_variable stopped_cond_;
    AudioFormat input_format_;
    AudioFormat output_format_;
    AlignedBuffer<int8_t> sample_buffer_;
    Timer timer_;
    AlignPtr<FileStream> stream_;
    AlignPtr<DeviceType> device_type_;
    AlignPtr<Device> device_;
    std::weak_ptr<PlaybackStateAdapter> state_adapter_;
    AudioBuffer<int8_t> buffer_;
    WaitableTimer wait_timer_;
    AlignPtr<Resampler> resampler_;   
    AlignPtr<Equalizer> equalizer_;
    VmMemLock sample_buffer_lock_;
    EQBands eqsettings_;
    DeviceInfo device_info_;
    std::shared_future<void> stream_task_;
    SpscQueue<GaplessPlayMsgID> msg_queue_;
};

}
