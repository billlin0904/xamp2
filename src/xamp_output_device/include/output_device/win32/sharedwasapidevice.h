//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_WIN

#include <atomic>

#include <base/logger.h>
#include <base/platfrom_handle.h>
#include <output_device/ioutputdevice.h>
#include <output_device/win32/wasapiworkqueue.h>

namespace xamp::output_device::win32 {

class SharedWasapiDevice final : public IOutputDevice {
public:
	explicit SharedWasapiDevice(CComPtr<IMMDevice> const & device);

	virtual ~SharedWasapiDevice() override;

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

	void SetSchedulerService(std::wstring const & mmcss_name, MmcssThreadPriority thread_priority);

	uint32_t GetBufferSize() const noexcept override;

	bool IsHardwareControlVolume() const override;

	void AbortStream() noexcept override;

private:
	HRESULT GetSampleRequested(bool is_silence) noexcept;

	HRESULT GetSample(uint32_t frame_available, bool is_silence) noexcept;

	void ReportError(HRESULT hr);

	void UnRegisterDeviceVolumeChange();

	void RegisterDeviceVolumeChange();

	void InitialRawMode(AudioFormat const & output_format);

	void InitialDeviceFormat(AudioFormat const & output_format);

	HRESULT OnInvoke(IMFAsyncResult* async_result);

	class DeviceEventNotification;

	std::atomic<bool> is_running_;
	std::atomic<int64_t> stream_time_;	
	uint32_t buffer_frames_;
	REFERENCE_TIME buffer_time_;
	std::wstring mmcss_name_;
	MmcssThreadPriority thread_priority_;
	WinHandle sample_ready_;
	CComHeapPtr<WAVEFORMATEX> mix_format_;
	CComPtr<IMMDevice> device_;
	CComPtr<IAudioClient3> client_;
	CComPtr<IAudioClock> clock_;
	CComPtr<IAudioRenderClient> render_client_;
	CComPtr<DeviceEventNotification> device_volume_notification_;
	CComPtr<IAudioEndpointVolume> endpoint_volume_;
	IAudioCallback* callback_;
	CComPtr<WASAPIWorkQueue<SharedWasapiDevice>> rt_work_queue_;
	std::shared_ptr<Logger> log_;
};

}

#endif


