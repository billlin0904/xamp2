//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/buffer.h>
#include <base/fastconditionvariable.h>
#include <base/threadpool.h>
#include <base/logger.h>
#include <base/task.h>

#include <output_device/idsddevice.h>
#include <output_device/ioutputdevice.h>

XAMP_OUTPUT_DEVICE_NAMESPACE_BEGIN

XAMP_DECLARE_LOG_NAME(NullOutputDevice);

class NullOutputDevice final : public IOutputDevice, public IDsdDevice {
public:
	explicit NullOutputDevice(const std::shared_ptr<IThreadPool>& thread_pool);

	virtual ~NullOutputDevice() override;

	void openStream(AudioFormat const& output_format) override;

	void setAudioCallback(IAudioCallback* callback) override;

	bool isStreamOpen() const override;

	bool isStreamRunning() const override;

	void stopStream(bool wait_for_stop_stream = true) override;

	void closeStream() override;

	void startStream() override;

	void setStreamTime(double stream_time) override;

	double getStreamTime() const override;

	uint32_t getVolume() const override;

	bool isMuted() const override;

	void setVolume(uint32_t volume) const override;

	void setMute(bool mute) const override;

	PackedFormat getPackedFormat() const override;

	uint32_t getBufferSize() const override;

	bool isHardwareControlVolume() const override;

	void abortStream() override;

	void setIoFormat(DsdIoFormat format) override;

	[[nodiscard]] virtual DsdIoFormat getIoFormat() const override;

private:
	bool is_running_;
	bool raw_mode_;
	bool is_playing_;
	mutable bool is_muted_;
	std::atomic<bool> is_stopped_;
	mutable uint32_t volume_;
	uint32_t buffer_frames_;
	std::atomic<int64_t> stream_time_;
	IAudioCallback* callback_;
	Future<void> render_task_;
	std::chrono::milliseconds wait_time_;
	LoggerPtr logger_;
	AudioFormat output_format_;
	Buffer<float> buffer_;
	std::shared_ptr<IThreadPool> thread_pool_;
};

XAMP_OUTPUT_DEVICE_NAMESPACE_END
