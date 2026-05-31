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
class IThreadPoolExecutor;
}

struct pw_buffer;

XAMP_OUTPUT_DEVICE_POSIX_NAMESPACE_BEGIN

XAMP_DECLARE_LOG_NAME(PipeWireOutputDevice);

class PipeWireOutputDevice final : public IOutputDevice {
public:
	explicit PipeWireOutputDevice(const std::shared_ptr<xamp::base::IThreadPoolExecutor>& thread_pool,
		std::string device_id);

	~PipeWireOutputDevice() override;

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
	static void CoreDoneCallback(void* userdata, uint32_t id, int seq);

	static void StreamStateCallback(void* userdata,
		pw_stream_state old_state,
		pw_stream_state state,
		const char* error);

	static void StreamProcessCallback(void* userdata);

	void WaitForCoreReady();

	void WaitForStreamReady();

	void OnStreamProcess();

	void ApplySoftwareVolume(float* samples, size_t sample_count) const;

	void ConfigureRealtimeThreadPriority();

	[[nodiscard]] double GetCallbackStreamTime(int64_t next_frame) const;

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
