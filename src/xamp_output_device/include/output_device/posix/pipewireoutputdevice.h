//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <atomic>
#include <memory>
#include <string>

#include <base/base.h>
#include <base/logger.h>

#include <output_device/ioutputdevice.h>
#include <output_device/posix/pipewire_private.h>

namespace xamp::base {
class IThreadPool;
}

struct pw_buffer;

XAMP_OUTPUT_DEVICE_POSIX_NAMESPACE_BEGIN

XAMP_DECLARE_LOG_NAME(PipeWireOutputDevice);

class PipeWireOutputDevice final : public IOutputDevice {
public:
	explicit PipeWireOutputDevice(const std::shared_ptr<xamp::base::IThreadPool>& thread_pool,
		std::string device_id);

	~PipeWireOutputDevice() override;

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
	static void coreDoneCallback(void* userdata, uint32_t id, int seq);

	static void streamStateCallback(void* userdata,
		pw_stream_state old_state,
		pw_stream_state state,
		const char* error);

	static void streamProcessCallback(void* userdata);

	void waitForCoreReady();

	void waitForStreamReady();

	void onStreamProcess();

	void applySoftwareVolume(float* samples, size_t sample_count) const;

	void configureRealtimeThreadPriority();

	[[nodiscard]] double getCallbackStreamTime(int64_t next_frame) const;

	std::atomic<bool> is_running_{ false };
	std::atomic<bool> is_stopped_{ true };
	mutable std::atomic<bool> is_muted_{ false };
	std::atomic<bool> realtime_priority_configured_{ false };
	std::atomic<int64_t> stream_frame_{ 0 };
	std::atomic<int64_t> stream_time_offset_frame_{ 0 };
	std::string device_id_;
	AudioFormat output_format_;
	LoggerPtr logger_;
	IAudioCallback* callback_{ nullptr };
	PipeWireThreadLoopPtr loop_;
	PipeWireContextPtr context_;
	PipeWireCorePtr core_;
	PipeWireStreamPtr stream_;
	spa_hook core_listener_{};
	spa_hook stream_listener_{};
	int core_sync_seq_{ 0 };
	bool core_ready_{ false };
	bool stream_ready_{ false };
	mutable std::atomic<uint32_t> volume_{ 100 };
	uint32_t buffer_frames_{ 0 };
};

XAMP_OUTPUT_DEVICE_POSIX_NAMESPACE_END
