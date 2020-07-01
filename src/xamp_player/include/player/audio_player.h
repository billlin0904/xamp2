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
#include <base/id.h>

#include <stream/stream.h>
#include <stream/equalizer.h>
#include <stream/bassequalizer.h>

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
    XAMP_DISABLE_COPY(AudioPlayer)

    AudioPlayer();

    virtual ~AudioPlayer() override;

    explicit AudioPlayer(std::weak_ptr<PlaybackStateAdapter> adapter);

    static void LoadLib();

    void Open(std::wstring const& file_path, std::wstring const& file_ext, const DeviceInfo& device_info);

    void StartPlay();

    void Play();

    void Pause();

    void Resume();

    void Stop(bool signal_to_stop = true, bool shutdown_device = false, bool wait_for_stop_stream = true);

    void Destroy();

    void Seek(double stream_time);

    void SetVolume(uint32_t volume);

    uint32_t GetVolume() const;

    bool CanHardwareControlVolume() const;

    bool IsMute() const;

    void SetMute(bool mute);

    bool IsPlaying() const noexcept;

    DsdModes GetDsdModes() const noexcept;

    bool IsDSDFile() const;

    std::optional<uint32_t> GetDSDSpeed() const;

    std::optional<DeviceInfo> GetDefaultDeviceInfo() const;

    double GetDuration() const;

    PlayerState GetState() const noexcept;

    AudioFormat GetStreamFormat() const noexcept;

    AudioFormat GetOutputFormat() const noexcept;

    bool IsDsdStream() const noexcept;

    void SetResampler(uint32_t samplerate, AlignPtr<Resampler> &&resampler);

    void EnableResampler(bool enable = true);

    void SetEQ(uint32_t band, float gain, float Q);

    static AlignPtr<FileStream> MakeFileStream(std::wstring const& file_ext);
private:
    void Initial();

    void OpenStream(std::wstring const & file_path, std::wstring const & file_ext, DeviceInfo const& device_info);

    void CreateDevice(ID const& device_type_id, std::wstring const & device_id, bool open_always);

    void CloseDevice(bool wait_for_stop_stream);

    void CreateBuffer();

    void SetDeviceFormat();

    void BufferStream();

    int32_t OnGetSamples(void* samples, uint32_t num_buffer_frames, double stream_time) noexcept override;

    void OnVolumeChange(float vol) noexcept override;

    void OnError(Exception const & e) noexcept override;

    void OnDeviceStateChange(DeviceState state, std::wstring const & device_id) override;

    void OpenDevice(double stream_time = 0.0);

    void SetState(PlayerState play_state);

    void ReadSampleLoop(int8_t* sample_buffer, uint32_t max_read_sample, std::unique_lock<std::mutex> &lock);

    DsdStream* AsDsdStream() noexcept;

    DsdDevice* AsDsdDevice() noexcept;

    void UpdateSlice(float const *samples = nullptr, int32_t sample_size = 0, double stream_time = 0.0) noexcept;

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

    bool is_muted_;
    bool enable_resample_;
    DsdModes dsd_mode_;
    std::atomic<PlayerState> state_;
    uint32_t target_samplerate_;
    uint32_t volume_;
    uint32_t num_buffer_samples_;
    uint32_t num_read_sample_;
    uint32_t read_sample_size_;
    uint32_t sample_size_;
    std::atomic<bool> is_playing_;
    std::atomic<bool> is_paused_;
    std::atomic<AudioSlice> slice_;
    mutable std::mutex pause_mutex_;
    std::wstring device_id_;
    ID device_type_id_;
    std::condition_variable pause_cond_;
    std::condition_variable stopped_cond_;
    AudioFormat input_format_;
    AudioFormat output_format_;
    AlignBufferPtr<int8_t> sample_buffer_;
    Timer timer_;
    AlignPtr<FileStream> stream_;
    AlignPtr<DeviceType> device_type_;
    AlignPtr<Device> device_;
    std::weak_ptr<PlaybackStateAdapter> state_adapter_;
    AudioBuffer<int8_t> buffer_;
    WaitableTimer wait_timer_;
    AlignPtr<Resampler> resampler_;
    AlignPtr<Equalizer> equalizer_;
    std::array<EQSettings, kMaxBand> eqsettings_;
    DeviceInfo device_info_;
    std::future<void> stream_task_;
};

}
