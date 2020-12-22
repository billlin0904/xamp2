//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <mutex>
#include <atomic>

#include <base/base.h>

#ifdef XAMP_OS_WIN

#include <base/vmmemlock.h>
#include <base/logger.h>
#include <output_device/device.h>

namespace xamp::output_device::win32 {

class XAMP_OUTPUT_DEVICE_API SharedWasapiDevice final : public Device {
public:
	explicit SharedWasapiDevice(CComPtr<IMMDevice> const & device);

	virtual ~SharedWasapiDevice();

	void OpenStream(AudioFormat const & output_format) override;

	void SetAudioCallback(AudioCallback* callback) noexcept override;

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

	void DisplayControlPanel() override;

	PackedFormat GetPackedFormat() const noexcept override;

	void SetSchedulerService(std::wstring const & mmcss_name, MmcssThreadPriority thread_priority);

	uint32_t GetBufferSize() const noexcept override;

	bool IsHardwareControlVolume() const override;

	void AbortStream() noexcept override;

private:
	void ReportError(HRESULT hr);

	void UnRegisterDeviceVolumeChange();

	void RegisterDeviceVolumeChange();

	void InitialRawMode(AudioFormat const & output_format);

	void InitialDeviceFormat(AudioFormat const & output_format);

	void GetSampleRequested(bool is_silence) noexcept;

	void GetSample(uint32_t frame_available) noexcept;

	void FillSilentSample(uint32_t frame_available) noexcept;

	HRESULT OnSampleReady(IMFAsyncResult* result) noexcept;

	class DeviceEventNotification;

	std::atomic<bool> is_running_;
	std::atomic<bool> is_stop_streaming_;
	std::atomic<int64_t> stream_time_;
	DWORD queue_id_;
	uint32_t latency_;
	uint32_t buffer_frames_;
	std::wstring mmcss_name_;
	MmcssThreadPriority thread_priority_;
	mutable std::mutex mutex_;
	std::condition_variable condition_;
	MFWORKITEM_KEY sample_ready_key_;
	WinHandle sample_ready_;
	CComHeapPtr<WAVEFORMATEX> mix_format_;
	CComPtr<IMMDevice> device_;
	CComPtr<IAudioClient3> client_;
	CComPtr<IAudioClock> clock_;
	CComPtr<IAudioRenderClient> render_client_;
	CComPtr<MFAsyncCallback<SharedWasapiDevice>> sample_ready_callback_;
	CComPtr<IMFAsyncResult> sample_ready_async_result_;	
	CComPtr<DeviceEventNotification> device_volume_notification_;
	AudioCallback* callback_;
	std::shared_ptr<spdlog::logger> log_;
};

}

#endif


