//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <cstdint>
#include <mutex>
#include <future>
#include <optional>
#include <filesystem>

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

#ifdef _DEBUG
#include <base/stopwatch.h>
#endif

#include <output_device/audiodevicemanager.h>
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

    explicit AudioPlayer(const std::weak_ptr<PlaybackStateAdapter>& adapter);

    static void Initial();

    void Startup();

    void Open(std::filesystem::path const& file_path);

    void Open(std::filesystem::path const& file_path, const DeviceInfo& device_info);       

    void PrepareToPlay(double start_time = 0.0, double end_time = 0.0);

    void Play();

    void Pause();

    void Resume();

    void Stop(bool signal_to_stop = true, bool shutdown_device = false, bool wait_for_stop_stream = true);

    void Destroy();
    	
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

    AudioFormat GetInputFormat() const noexcept;

    AudioFormat GetOutputFormat() const noexcept;

    void SetSampleRateConverter(uint32_t sample_rate, AlignPtr<SampleRateConverter> && converter);

    void SetProcessor(AlignPtr<AudioProcessor> &&processor);

    void EnableProcessor(bool enable = true);

    bool IsEnableProcessor() const;

    void EnableSampleRateConverter(bool enable = true);

    bool IsEnableSampleRateConverter() const;

    void SetDevice(const DeviceInfo& device_info);

    DeviceInfo GetDevice() const;

    bool IsGaplessPlay() const;

    void EnableGaplessPlay(bool enable);

    void ClearPlayQueue() const;   

    AlignPtr<SampleRateConverter> CloneSampleRateConverter() const;

    AudioDeviceManager& GetAudioDeviceManager();

private:
    enum class MsgID {
        EVENT_SWITCH,
    };

    bool CanProcessFile() const noexcept;
    	
    void DoSeek(double stream_time);        
    	
    void OpenStream(std::filesystem::path const& file_path, DeviceInfo const& device_info);

    void CreateDevice(Uuid const& device_type_id, std::string const & device_id, bool open_always);

    void CloseDevice(bool wait_for_stop_stream);

    void CreateBuffer();

    void SetDeviceFormat();

    void BufferStream(double stream_time = 0.0);

    DataCallbackResult OnGetSamples(void* samples, size_t num_buffer_frames, double stream_time, double sample_time) noexcept override;

    void OnVolumeChange(float vol) noexcept override;

    void OnError(Exception const & e) noexcept override;

    void OnDeviceStateChange(DeviceState state, std::string const & device_id) override;

    void OpenDevice(double stream_time = 0.0);

    void SetState(PlayerState play_state);

    void ReadSampleLoop(int8_t* sample_buffer, uint32_t max_buffer_sample, std::unique_lock<std::mutex> &lock);

    void BufferSamples(AlignPtr<FileStream>& stream, AlignPtr<SampleRateConverter> &converter, int32_t buffer_count = 1);

    void UpdateSlice(size_t sample_size = 0, double stream_time = 0.0) noexcept;

    void OnGaplessPlayState(std::unique_lock<std::mutex>& lock);

    void AllocateReadBuffer(uint32_t allocate_size);

    void ResizeFifo();

    void ProcessEvent();

    void InitProcessor();

#ifdef _DEBUG
    void CheckRace();
#endif

    struct XAMP_CACHE_ALIGNED(kMallocAlignSize) AudioSlice {
	    explicit AudioSlice(size_t sample_size = 0,
	        double stream_time = 0.0) noexcept;        
        size_t sample_size;
        double stream_time;
    };

    XAMP_ENFORCE_TRIVIAL(AudioSlice)    

    bool is_muted_;
    bool enable_sample_converter_;
    bool enable_processor_;
    bool is_dsd_file_;
    DsdModes dsd_mode_;
    std::atomic<PlayerState> state_;
    uint8_t sample_size_;
    uint32_t target_sample_rate_;
    uint32_t volume_;
    uint32_t fifo_size_;
    uint32_t num_read_sample_;
    std::optional<uint32_t> dsd_speed_;
    std::atomic<bool> is_playing_;
    std::atomic<bool> is_paused_;
    std::atomic<bool> enable_gapless_play_;
    std::atomic<double> sample_end_time_;
    std::atomic<double> stream_duration_;
    std::atomic<AudioSlice> slice_;
    mutable std::mutex pause_mutex_;
    mutable std::mutex stream_read_mutex_;
#ifdef _DEBUG
    std::chrono::microseconds max_process_time_{ 0 };
    Stopwatch sw_;
    FastMutex debug_mutex_;
    std::string render_thread_id_;
#endif    
    std::string device_id_;
    Uuid device_type_id_;
    std::condition_variable pause_cond_;
    std::condition_variable stopped_cond_;
    AudioFormat input_format_;
    AudioFormat output_format_;    
    Timer timer_;
    AudioDeviceManager device_manager_;
    AlignPtr<FileStream> stream_;
    AlignPtr<DeviceType> device_type_;
    AlignPtr<Device> device_;
    std::weak_ptr<PlaybackStateAdapter> state_adapter_;    
    AudioBuffer<int8_t> fifo_;
    Buffer<int8_t> read_buffer_;
    WaitableTimer wait_timer_;
    AlignPtr<SampleRateConverter> converter_;
    std::vector<AlignPtr<AudioProcessor>> dsp_chain_;
    DeviceInfo device_info_;    
    std::shared_future<void> stream_task_;
    SpscQueue<MsgID> msg_queue_;
    SpscQueue<double> seek_queue_;
    SpscQueue<AlignPtr<AudioProcessor>> processor_queue_;
};

}
