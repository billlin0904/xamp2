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
#include <base/threadpool.h>
#include <base/unique_handle.h>
#include <base/logger.h>

#include <output_device/ioutputdevice.h>

struct _snd_pcm;
using snd_pcm_t = _snd_pcm;

XAMP_OUTPUT_DEVICE_POSIX_NAMESPACE_BEGIN

XAMP_DECLARE_LOG_NAME(AlsaOutputDevice);

class AlsaOutputDevice final : public IOutputDevice {
public:
	explicit AlsaOutputDevice(const std::shared_ptr<IThreadPool>& thread_pool,
		std::string device_id);

	~AlsaOutputDevice() override;

	void openStream(const AudioFormat& output_format) override;

	void setAudioCallback(IAudioCallback* callback) override;

	[[nodiscard]] bool isStreamOpen() const override;

	[[nodiscard]] bool isStreamRunning() const override;

	void stopStream(bool wait_for_stop_stream = true) override;

	void closeStream() override;

	void startStream() override;

	void setStreamTime(double stream_time) override;

	[[nodiscard]] double getStreamTime() const override;

	[[nodiscard]] uint32_t getVolume() const override;

	void setVolume(uint32_t volume) const override;

	void setMute(bool mute) const override;

	[[nodiscard]] bool isMuted() const override;

	[[nodiscard]] bool isHardwareControlVolume() const override;

	[[nodiscard]] PackedFormat getPackedFormat() const override;

	[[nodiscard]] uint32_t getBufferSize() const override;

	void abortStream() override;

private:
	struct SndPcmHandleTraits final {
		static snd_pcm_t* invalid() {
			return nullptr;
		}
		static void close(snd_pcm_t* value);
	};

	using SndPcmHandle = UniqueHandle<snd_pcm_t*, SndPcmHandleTraits>;

	void renderLoop(const std::stop_token& stop_token);

	void stopRenderThread(bool wait_for_stop_stream);

	void applySoftwareVolume(float* samples, size_t sample_count) const;

	void configureRealtimeThreadPriority();

	void handleRecoverableError(int error);

	[[nodiscard]] bool isRecoverableState() const;

	[[nodiscard]] bool waitUntilWritable(const std::stop_token& stop_token);

	[[nodiscard]] double getCallbackStreamTime(int64_t next_frame) const;
	
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
	std::shared_ptr<IThreadPool> thread_pool_;
	LoggerPtr logger_;
};

XAMP_OUTPUT_DEVICE_POSIX_NAMESPACE_END
