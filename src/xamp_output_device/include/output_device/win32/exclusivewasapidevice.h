//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#ifdef XAMP_OS_WIN

#include <atomic>

#include <base/logger.h>
#include <base/dataconverter.h>
#include <base/buffer.h>
#include <base/ithreadpool.h>

#include <output_device/win32/wasapi.h>
#include <output_device/idsddevice.h>
#include <output_device/ioutputdevice.h>

namespace xamp::output_device::win32 {

class ExclusiveWasapiDevice final
	: public IOutputDevice
	, public IDsdDevice {
public:
	explicit ExclusiveWasapiDevice(CComPtr<IMMDevice> const & device);

	virtual ~ExclusiveWasapiDevice() override;

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

	bool IsMuted() const override;

	PackedFormat GetPackedFormat() const noexcept override;

	void SetSchedulerService(std::wstring const & mmcss_name, MmcssThreadPriority thread_priority);

	uint32_t GetBufferSize() const noexcept override;

	bool IsHardwareControlVolume() const override;

	void AbortStream() noexcept override;

	void SetIoFormat(DsdIoFormat format) override;

	DsdIoFormat GetIoFormat() const override;

private:

	void InitialDeviceFormat(AudioFormat const & output_format, const uint32_t valid_bits_samples);

	void SetAlignedPeriod(REFERENCE_TIME device_period, AudioFormat const & output_format);

	void ReportError(HRESULT hr) noexcept;

	bool GetSample(bool is_silence) noexcept;
	
	bool raw_mode_;
	std::atomic<bool> is_running_;
	MmcssThreadPriority thread_priority_;
	uint32_t buffer_frames_;
	uint32_t buffer_duration_ms_;
	REFERENCE_TIME buffer_period_;
	DWORD volume_support_mask_;
	std::atomic<int64_t> stream_time_;
	WinHandle sample_ready_;
	WinHandle thread_start_;
	WinHandle thread_exit_;
	WinHandle close_request_;
	std::wstring mmcss_name_;
	AudioConvertContext data_convert_;
	REFERENCE_TIME aligned_period_;
	CComPtr<IMMDevice> device_;
	CComPtr<IAudioClient2> client_;
	CComPtr<IAudioRenderClient> render_client_;
	CComPtr<IAudioEndpointVolume> endpoint_volume_;
	CComPtr<IAudioClock> clock_;
	CComHeapPtr<WAVEFORMATEX> mix_format_;
	Buffer<float> buffer_;
	IAudioCallback* callback_;
	std::shared_ptr<Logger> log_;
	SharedFuture<void> render_task_;
};

}

#endif
