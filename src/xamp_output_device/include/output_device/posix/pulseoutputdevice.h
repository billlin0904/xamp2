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
#include <output_device/posix/pulse_private.h>

namespace xamp::base {
class IThreadPool;
}

struct pa_context;
struct pa_operation;
struct pa_stream;
struct pa_threaded_mainloop;

XAMP_OUTPUT_DEVICE_POSIX_NAMESPACE_BEGIN

XAMP_DECLARE_LOG_NAME(PulseOutputDevice);

class PulseOutputDevice final : public IOutputDevice {
public:
	explicit PulseOutputDevice(const std::shared_ptr<xamp::base::IThreadPool>& thread_pool,
		std::string device_id);

	~PulseOutputDevice() override;

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

	[[nodiscard]] bool isMuted() const override;

	void setVolume(uint32_t volume) const override;

	void setMute(bool mute) const override;

	[[nodiscard]] PackedFormat getPackedFormat() const override;

	[[nodiscard]] uint32_t getBufferSize() const override;

	[[nodiscard]] bool isHardwareControlVolume() const override;

	void abortStream() override;

private:
	static void contextStateCallback(pa_context* context, void* userdata);

	static void streamStateCallback(pa_stream* stream, void* userdata);

	static void streamWriteCallback(pa_stream* stream, size_t bytes, void* userdata);

	static void streamSuccessCallback(pa_stream* stream, int success, void* userdata);

	void waitForContextReady() const;

	void waitForStreamReady() const;

	void waitForOperation(pa_operation* operation) const;

	void onStreamWrite(pa_stream* stream, size_t bytes);

	void applySoftwareVolume(float* samples, size_t sample_count) const;

	void resetPulseTimeBase();

	[[nodiscard]] double getCallbackStreamTime(int64_t fallback_frame) const;

	void configureRealtimeThreadPriority();

	std::string device_id_;
	AudioFormat output_format_;
	LoggerPtr logger_;
	IAudioCallback* callback_{ nullptr };
	PulseThreadedMainloopPtr mainloop_;
	PulseContextPtr context_;
	PulseStreamPtr stream_;
	std::atomic<bool> is_running_{ false };
	std::atomic<bool> is_stopped_{ true };
	mutable std::atomic<bool> is_muted_{ false };
	mutable std::atomic<uint32_t> volume_{ 100 };
	std::atomic<int64_t> stream_frame_{ 0 };
	std::atomic<int64_t> stream_time_offset_frame_{ 0 };
	std::atomic<int64_t> pulse_time_base_usec_{ -1 };
	std::atomic<bool> realtime_priority_configured_{ false };
	uint32_t buffer_frames_{ 0 };
};

XAMP_OUTPUT_DEVICE_POSIX_NAMESPACE_END
