//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <mutex>
#include <future>
#include <optional>

#include <base/audiobuffer.h>
#include <base/audioformat.h>
#include <base/timer.h>
#include <base/dsdsampleformat.h>
#include <base/align_ptr.h>
#include <base/vmmemlock.h>
#include <base/threadpool.h>

#include <stream/filestream.h>

#include <output_device/audiocallback.h>
#include <output_device/device.h>
#include <output_device/device_type.h>

#include <player/playstate.h>
#include <player/playbackstateadapter.h>
#include <player/player.h>

namespace xamp::player {

using namespace stream;
using namespace output_device;

class XAMP_PALYER_API AudioPlayer :
	public AudioCallback,
	public std::enable_shared_from_this<AudioPlayer> {
public:
	XAMP_DISABLE_COPY(AudioPlayer)

	AudioPlayer();

	explicit AudioPlayer(std::weak_ptr<PlaybackStateAdapter> adapter);

	virtual ~AudioPlayer();

	void Open(const std::wstring& file_path, bool use_bass_stream, const DeviceInfo& device_info);

	void PlayStream();

	void Play();

	void Pause();

	void Resume();

	void Stop(bool signal_to_stop = true, bool shutdown_device = false);

	void Seek(double stream_time);

	void SetVolume(int32_t volume);

	int32_t GetVolume() const;

	bool IsMute() const;

	void SetMute(bool mute);

	bool IsPlaying() const;

	DSDModes GetDSDModes() const;

	std::optional<DeviceInfo> GetDefaultDeviceInfo() const;

	double GetDuration() const;

	PlayerState GetState() const noexcept;

	AudioFormat GetStreamFormat() const;
private:
	void Initial();

	void OpenStream(const std::wstring& file_path, bool use_bass_stream, const DeviceInfo& device_info);

	void CreateDevice(const ID& device_type_id, const std::wstring& device_id, const bool open_always);

	void CloseDevice();

	void CreateBuffer();

	void SetDeviceFormat();

	void BufferStream();

	bool ProcessSamples(int32_t num_samples) noexcept;

	int operator()(void* samples, const int32_t num_buffer_frames, const double stream_time) noexcept override;

	void OnError(const Exception& e) noexcept override;

	void OpenDevice(double stream_time = 0.0);

	void SetState(const PlayerState play_state);

	struct alignas(XAMP_CACHE_ALIGN_SIZE) AudioSlice {
		AudioSlice(const float* samples = nullptr, int32_t sample_size = 0, double stream_time = 0.0) noexcept
			: samples(samples)
			, sample_size(sample_size)
			, stream_time(stream_time) {
		}

		bool operator==(const AudioSlice& other) const noexcept {
			return samples == other.samples
				&& sample_size == other.sample_size
				&& stream_time == other.stream_time;
		}

		const float* samples;
		int32_t sample_size;
		double stream_time;
	};

	bool is_muted_;
	DSDModes dsd_mode_;
	PlayerState state_;
	int32_t volume_;
	int32_t num_buffer_samples_;
	int32_t num_read_sample_;
	int32_t read_sample_size_;
	std::atomic<bool> is_playing_;
	std::atomic<bool> is_paused_;
	std::atomic<AudioSlice> slice_;
	mutable AudioSlice cache_slice_;
	mutable std::mutex pause_mutex_;
	std::wstring device_id_;
	ID device_type_id_;
	std::condition_variable pause_cond_;
	std::condition_variable stopped_cond_;
	AudioFormat input_format_;
	AudioFormat output_format_;
	AlignBufferPtr<int8_t> read_sample_buffer_;
	AlignPtr<Timer> timer_;
	AlignPtr<FileStream> stream_;
	AlignPtr<DeviceType> device_type_;
	AlignPtr<Device> device_;
	std::weak_ptr<PlaybackStateAdapter> state_adapter_;
	AudioBuffer<int8_t> buffer_;
	VmMemLock vmlock_;
	std::future<void> stream_task_;
	ThreadPool thread_pool_;
};

}