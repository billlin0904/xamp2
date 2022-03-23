//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#if ENABLE_ASIO
#include <atomic>
#include <vector>

#include <asio.h>

#include <base/logger.h>
#include <base/dsdsampleformat.h>
#include <base/buffer.h>
#include <base/fastmutex.h>
#include <base/fastconditionvariable.h>
#include <output_device/ioutputdevice.h>
#include <output_device/idsddevice.h>
#include <output_device/win32/mmcss.h>

namespace xamp::output_device {

class AsioDevice final
	: public IOutputDevice
	, public IDsdDevice {
public:
	explicit AsioDevice(std::string const & device_id);

	virtual ~AsioDevice() override;

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

	void SetVolume(uint32_t volume) const override;

	void SetMute(bool mute) const override;

	void DisplayControlPanel() override;

	PackedFormat GetPackedFormat() const noexcept override;

	void SetIoFormat(DsdIoFormat format) override;

	DsdIoFormat GetIoFormat() const override;

	void SetSampleFormat(DsdFormat format);

	DsdFormat GetSampleFormat() const noexcept;

	uint32_t GetBufferSize() const noexcept override;

	bool IsMuted() const override;

	bool IsHardwareControlVolume() const override;

	void AbortStream() noexcept override;

	void ReOpen();

	static void ResetDriver();
private:
	static ASIOTime* OnBufferSwitchTimeInfoCallback(ASIOTime* timeInfo, long index, ASIOBool processNow) noexcept;

	static void OnBufferSwitchCallback(long index, ASIOBool processNow);

	static void OnSampleRateChangedCallback(ASIOSampleRate sampleRate);

	static long OnAsioMessagesCallback(long selector, long value, void* message, double* opt);

	void SetOutputSampleRate(AudioFormat const & output_format);

	void CreateBuffers(AudioFormat const & output_format);

	void OnBufferSwitch(long index, double sample_time) noexcept;

	std::tuple<int32_t, int32_t> GetDeviceBufferSize() const;

	bool IsSupportDsdFormat() const;

	void FillSlientData();

	bool is_removed_driver_;
	std::atomic<bool> is_stopped_;
	std::atomic<bool> is_streaming_;
	std::atomic<bool> is_stop_streaming_;
	long latency_;
	DsdIoFormat io_format_;
	DsdFormat sample_format_;
	mutable std::atomic<uint32_t> volume_;
	size_t buffer_size_;
	size_t buffer_bytes_;
	std::atomic<int64_t> played_bytes_;
	std::string device_id_;
	mutable FastMutex mutex_;
	FastConditionVariable condition_;
	AudioFormat format_;
	std::vector<ASIOClockSource> clock_source_;	
	Buffer<int8_t> buffer_;
	Buffer<int8_t> device_buffer_;	
	IAudioCallback* callback_;
	win32::Mmcss mmcss_;
	std::shared_ptr<spdlog::logger> log_;
};

}
#endif

