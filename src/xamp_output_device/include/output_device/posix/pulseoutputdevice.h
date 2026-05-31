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
class IThreadPoolExecutor;
}

struct pa_context;
struct pa_operation;
struct pa_stream;
struct pa_threaded_mainloop;

XAMP_OUTPUT_DEVICE_POSIX_NAMESPACE_BEGIN

XAMP_DECLARE_LOG_NAME(PulseOutputDevice);

class PulseOutputDevice final : public IOutputDevice {
public:
	explicit PulseOutputDevice(const std::shared_ptr<xamp::base::IThreadPoolExecutor>& thread_pool,
		std::string device_id);

	~PulseOutputDevice() override;

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

	[[nodiscard]] bool IsMuted() const override;

	void SetVolume(uint32_t volume) const override;

	void SetMute(bool mute) const override;

	[[nodiscard]] PackedFormat GetPackedFormat() const override;

	[[nodiscard]] uint32_t GetBufferSize() const override;

	[[nodiscard]] bool IsHardwareControlVolume() const override;

	void AbortStream() override;

private:
	static void ContextStateCallback(pa_context* context, void* userdata);

	static void StreamStateCallback(pa_stream* stream, void* userdata);

	static void StreamWriteCallback(pa_stream* stream, size_t bytes, void* userdata);

	static void StreamSuccessCallback(pa_stream* stream, int success, void* userdata);

	void WaitForContextReady() const;

	void WaitForStreamReady() const;

	void WaitForOperation(pa_operation* operation) const;

	void OnStreamWrite(pa_stream* stream, size_t bytes);

	void ApplySoftwareVolume(float* samples, size_t sample_count) const;

	void ResetPulseTimeBase();

	[[nodiscard]] double GetCallbackStreamTime(int64_t fallback_frame) const;

	void ConfigureRealtimeThreadPriority();

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
