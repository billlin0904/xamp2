//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
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

#include <output_device/output_device.h>
#include <output_device/deviceinfo.h>

#include <stream/stream.h>
#include <stream/iequalizer.h>

#include <player/player.h>
#include <player/playstate.h>
#include <player/iaudioplayer.h>

namespace xamp::player {

using namespace xamp::base;
using namespace xamp::stream;
using namespace xamp::output_device;

enum class DspCommandId {
    DSP_EQ,
    DSP_PREAMP,
};

struct DspMessage {
    DspCommandId id;
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

    void Open(Path const& file_path, const Uuid& device_id = Uuid::kInvalidUUID) override;

    void Open(Path const& file_path, const DeviceInfo& device_info, uint32_t target_sample_rate = 0, AlignPtr<ISampleRateConverter> converter = nullptr)  override;

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

    PlayerState GetState() const noexcept override;

    AudioFormat GetInputFormat() const noexcept override;

    AudioFormat GetOutputFormat() const noexcept override;

    void AddProcessor(AlignPtr<IAudioProcessor> processor) override;

    void EnableProcessor(bool enable = true) override;

    void SetEq(uint32_t band, float gain, float Q) override;

    void SetEq(EQSettings const& settings) override;

    void SetPreamp(float preamp) override;

    bool IsEnableProcessor() const override;

    bool IsEnableSampleRateConverter() const override;

    void SetDevice(const DeviceInfo& device_info) override;

    DeviceInfo GetDevice() const override;

    const AlignPtr<IAudioDeviceManager>& GetAudioDeviceManager() override;
private:
    bool CanProcessFile() const noexcept;
    	
    void DoSeek(double stream_time);        
    	
    void OpenStream(Path const& file_path, DeviceInfo const& device_info);

    void CreateDevice(Uuid const& device_type_id, std::string const & device_id, bool open_always);

    void CloseDevice(bool wait_for_stop_stream);

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

    void BufferSamples(AlignPtr<FileStream>& stream, AlignPtr<ISampleRateConverter> &converter, int32_t buffer_count = 1);

    void UpdateSlice(int32_t sample_size = 0, double stream_time = 0.0) noexcept;

    void AllocateReadBuffer(uint32_t allocate_size);

    void AllocateFifo();

    void ProcessSeek();

    void InitDsp();

    void ProcessDspMsg();

    void SetDSDStreamMode(DsdModes dsd_mode, AlignPtr<FileStream>& stream);

    struct XAMP_CACHE_ALIGNED(kMallocAlignSize) AudioSlice {
	    explicit AudioSlice(int32_t sample_size = 0,
	        double stream_time = 0.0) noexcept;        
        int32_t sample_size;
        double stream_time;
    };

    template <typename Processor>
    Processor* GetProcessor() {
        auto itr = std::find_if(dsp_chain_.begin(),
                                dsp_chain_.end(),
                                [](auto const& processor) {
                                    return processor->GetTypeId() == Processor::Id;
                                });
        if (itr == dsp_chain_.end()) {
            return nullptr;
        }
        return dynamic_cast<Processor*>((*itr).get());
    }

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
    std::atomic<double> sample_end_time_;
    std::atomic<double> stream_duration_;
    std::atomic<AudioSlice> slice_;
    mutable FastMutex pause_mutex_;
    mutable FastMutex stopped_mutex_;
    EQSettings eq_settings_;
    std::string device_id_;
    Uuid device_type_id_;
    FastConditionVariable pause_cond_;
    FastConditionVariable read_finish_and_wait_seek_signal_cond_;
    AudioFormat input_format_;
    AudioFormat output_format_;    
    Timer timer_;
    AlignPtr<IAudioDeviceManager> device_manager_;
    AlignPtr<FileStream> stream_;
    AlignPtr<IDeviceType> device_type_;
    AlignPtr<IDevice> device_;
    std::weak_ptr<IPlaybackStateAdapter> state_adapter_;    
    AudioBuffer<int8_t> fifo_;
    Buffer<int8_t> read_buffer_;
    Buffer<float> dsp_buffer_;
    WaitableTimer wait_timer_;
    AlignPtr<ISampleRateConverter> converter_;
    std::vector<AlignPtr<IAudioProcessor>> setting_chain_;
    std::vector<AlignPtr<IAudioProcessor>> dsp_chain_;
    DeviceInfo device_info_;    
    std::shared_future<void> stream_task_;
    SpscQueue<double> seek_queue_;
    SpscQueue<DspMessage> dsp_msg_queue_;
    std::shared_ptr<spdlog::logger> logger_;
};

}
