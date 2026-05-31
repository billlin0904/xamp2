//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <atomic>
#include <future>
#include <memory>
#include <string>
#include <vector>

#include <poll.h>

#include <base/base.h>
#include <base/buffer.h>
#include <base/ithreadpoolexecutor.h>
#include <base/unique_handle.h>
#include <base/logger.h>

#include <output_device/ioutputdevice.h>

struct _snd_pcm;
using snd_pcm_t = _snd_pcm;

XAMP_OUTPUT_DEVICE_POSIX_NAMESPACE_BEGIN

XAMP_DECLARE_LOG_NAME(AlsaOutputDevice);

class AlsaOutputDevice final : public IOutputDevice {
public:
	explicit AlsaOutputDevice(const std::shared_ptr<IThreadPoolExecutor>& thread_pool,
		std::string device_id);

	~AlsaOutputDevice() override;

	void OpenStream(const AudioFormat& output_format) override;

	void SetAudioCallback(IAudioCallback* callback) override;

	[[nodiscard]] bool IsStreamOpen() const override;

	[[nodiscard]] bool IsStreamRunning() const override;

	void StopStream(bool wait_for_stop_stream = true) override;

	void CloseStream() override;

	void StartStream() override;

	void SetStreamTime(double stream_time) override;

	[[nodiscard]] double GetStreamTime() const override;

	[[nodiscard]] uint32_t GetVolume() const override;

	void SetVolume(uint32_t volume) const override;

	void SetMute(bool mute) const override;

	[[nodiscard]] bool IsMuted() const override;

	[[nodiscard]] bool IsHardwareControlVolume() const override;

	[[nodiscard]] PackedFormat GetPackedFormat() const override;

	[[nodiscard]] uint32_t GetBufferSize() const override;

	void AbortStream() override;

private:
	struct SndPcmHandleTraits final {
		static snd_pcm_t* invalid() {
			return nullptr;
		}
		static void Close(snd_pcm_t* value);
	};

	using SndPcmHandle = UniqueHandle<snd_pcm_t*, SndPcmHandleTraits>;

	void RenderLoop(const std::stop_token& stop_token);

	void StopRenderThread(bool wait_for_stop_stream);

	void ApplySoftwareVolume(float* samples, size_t sample_count) const;

	void ConfigureRealtimeThreadPriority();

	void HandleRecoverableError(int error);

	[[nodiscard]] bool IsRecoverableState() const;

	[[nodiscard]] bool WaitUntilWritable(const std::stop_token& stop_token);

	[[nodiscard]] double GetCallbackStreamTime(int64_t next_frame) const;
	
	std::atomic<bool> is_running_{ false };
	std::atomic<bool> stop_requested_{ true };
	mutable std::atomic<bool> is_muted_{ false };
	std::atomic<bool> realtime_priority_configured_{ false };
	std::atomic<int64_t> stream_time_offset_frame_{ 0 };
	std::atomic<int64_t> stream_frame_{ 0 };
	SndPcmHandle pcm_;
	IAudioCallback* callback_{ nullptr };
	AudioFormat output_format_;
	mutable std::atomic<uint32_t> volume_{ 100 };
	uint32_t buffer_frames_{ 0 };
	uint32_t device_buffer_frames_{ 0 };	
	std::vector<pollfd> poll_descriptors_;
	std::string device_id_;
	std::future<void> render_future_;
	Buffer<float> render_buffer_;
	std::shared_ptr<IThreadPoolExecutor> thread_pool_;
	LoggerPtr logger_;
};

XAMP_OUTPUT_DEVICE_POSIX_NAMESPACE_END
