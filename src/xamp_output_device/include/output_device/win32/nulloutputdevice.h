//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/buffer.h>
#include <base/task.h>

#ifdef XAMP_OS_WIN

#include <output_device/ioutputdevice.h>
#include <base/logger.h>

namespace xamp::output_device::win32 {

XAMP_DECLARE_LOG_NAME(NullOutputDevice);

class NullOutputDevice final : public IOutputDevice {
public:
	NullOutputDevice();

	virtual ~NullOutputDevice() override;

	void OpenStream(AudioFormat const & output_format) override;

	void SetAudioCallback(IAudioCallback* callback) noexcept override;

	bool IsStreamOpen() const noexcept override;

	bool IsStreamRunning() const noexcept override;

	void StopStream(bool wait_for_stop_stream = true) override;

	void CloseStream() override;

	void StartStream() override;

	void SetStreamTime(double stream_time) noexcept override;

	double GetStreamTime() const noexcept override;

	uint32_t GetVolume() const override;

	bool IsMuted() const override;

	void SetVolume(uint32_t volume) const override;

	void SetMute(bool mute) const override;

	PackedFormat GetPackedFormat() const noexcept override;

	uint32_t GetBufferSize() const noexcept override;

	bool IsHardwareControlVolume() const override;

	void AbortStream() noexcept override;

private:
	bool is_running_;
	mutable bool is_muted_;
	std::atomic<bool> is_stopped_;
	uint32_t buffer_frames_;
	std::atomic<int64_t> stream_time_;
	IAudioCallback* callback_;
	Buffer<float> buffer_;
	Task<void> render_task_;
	AudioFormat output_format_;
	LoggerPtr log_;
};

}

#endif


