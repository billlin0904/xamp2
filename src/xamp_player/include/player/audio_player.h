//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <cstdint>
#include <mutex>
#include <future>
#include <optional>

#include <base/audiobuffer.h>
#include <base/audioformat.h>
#include <base/timer.h>
#include <base/dsdsampleformat.h>
#include <base/align_ptr.h>
#include <base/vmmemlock.h>

#include <stream/audiostream.h>

#include <output_device/audiocallback.h>
#include <output_device/devicestatelistener.h>
#include <output_device/device.h>
#include <output_device/device_type.h>

#include <player/soxresampler.h>
#include <player/cdspresampler.h>

#include <player/playstate.h>
#include <player/playbackstateadapter.h>
#include <player/player.h>

namespace xamp::stream {
	class DsdStream;
	class FileStream;
}

namespace xamp::output_device {
	class DsdDevice;
}

namespace xamp::player {

class XAMP_PLAYER_API AudioPlayer final :
	public AudioCallback,
	public DeviceStateListener,
	public std::enable_shared_from_this<AudioPlayer> {
public:
	XAMP_DISABLE_COPY(AudioPlayer)

	AudioPlayer();

	explicit AudioPlayer(std::weak_ptr<PlaybackStateAdapter> adapter);

	~AudioPlayer() override;

	static void LoadLib();

	void Open(const std::wstring& file_path, const std::wstring& file_ext, const DeviceInfo& device_info);

    void PlayStream();

	void Play();

	void Pause();

	void Resume();

	void Stop(bool signal_to_stop = true, bool shutdown_device = false, bool wait_for_stop_stream = true);

	void Seek(double stream_time);

	void SetVolume(int32_t volume);

	int32_t GetVolume() const;

	bool IsMute() const;

	void SetMute(bool mute);

	bool IsPlaying() const;

	DsdModes GetDSDModes() const;

    std::optional<int32_t> GetDSDSpeed() const;

	std::optional<DeviceInfo> GetDefaultDeviceInfo() const;

	double GetDuration() const;

	PlayerState GetState() const noexcept;

	AudioFormat GetStreamFormat() const;

	bool IsDsdStream() const;

	void SetResampler(int32_t samplerate, AlignPtr<Resampler> &&resampler);

	void SetEnableResampler(bool enable = true);

private:
	void PrepareAllocate();

	void Initial();

    void OpenStream(const std::wstring& file_path, const std::wstring& file_ext, const DeviceInfo& device_info);

	void CreateDevice(const ID& device_type_id, const std::wstring& device_id, const bool open_always);

	void CloseDevice(bool wait_for_stop_stream);

	void CreateBuffer();

	void SetDeviceFormat();

	void BufferStream();

	bool FillSamples(int32_t num_samples) noexcept;

	int OnGetSamples(void* samples, const int32_t num_buffer_frames, const double stream_time) noexcept override;

	void OnError(const Exception& e) noexcept override;

	void OnDeviceStateChange(DeviceState state, const std::wstring& device_id) override;

	void OpenDevice(double stream_time = 0.0);

	void SetState(const PlayerState play_state);

	void ReadSampleLoop(int32_t max_read_sample, std::unique_lock<std::mutex> &lock) noexcept;

	DsdStream* AsDsdStream();

	FileStream* AsFileStream();

	DsdDevice* AsDsdDevice();	

	struct XAMP_CACHE_ALIGNED(XAMP_MALLOC_ALGIGN_SIZE) AudioSlice {
        AudioSlice(const float* samples = nullptr, 
			int32_t sample_size = 0,
			double stream_time = 0.0) noexcept
            : samples(samples)
            , sample_size(sample_size)
			, stream_time(stream_time) {
		}

		XAMP_ALWAYS_INLINE bool operator==(const AudioSlice& other) const noexcept {
			return samples == other.samples
				&& sample_size == other.sample_size
				&& stream_time == other.stream_time;
		}

		const float* samples;
        int32_t sample_size;
		double stream_time;
	};

	XAMP_ENFORCE_TRIVIAL(AudioSlice)

	bool is_muted_;
	bool enable_resample_;
	DsdModes dsd_mode_;
	std::atomic<PlayerState> state_;
	int32_t target_samplerate_;
	int32_t volume_;
	int32_t num_buffer_samples_;
	int32_t num_read_sample_;
	int32_t read_sample_size_;
	int32_t sample_size_;
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
	AlignPtr<AudioStream> stream_;
	AlignPtr<DeviceType> device_type_;
	AlignPtr<Device> device_;
	std::weak_ptr<PlaybackStateAdapter> state_adapter_;
	AudioBuffer<int8_t> buffer_;	
	WaitableTimer wait_timer_;
	AlignPtr<Resampler> resampler_;
	std::future<void> stream_task_;
};

}
