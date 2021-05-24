//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <mutex>
#include <atomic>

#include <base/base.h>

#ifdef XAMP_OS_WIN

#include <base/logger.h>
#include <base/fastmutex.h>
#include <output_device/device.h>

namespace xamp::output_device::win32 {

class SharedWasapiDevice final : public Device {
public:
	explicit SharedWasapiDevice(CComPtr<IMMDevice> const & device);

	virtual ~SharedWasapiDevice() override;

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

	HRESULT GetSampleRequested(bool is_silence) noexcept;

	HRESULT GetSample(uint32_t frame_available, bool is_silence) noexcept;

	HRESULT OnSampleReady(IMFAsyncResult* result);

	HRESULT OnStartPlayback(IMFAsyncResult* result);

	HRESULT OnPausePlayback(IMFAsyncResult* result);

	HRESULT OnStopPlayback(IMFAsyncResult* result);

	class DeviceEventNotification;

	std::atomic<bool> is_running_;
	std::atomic<bool> is_stop_require_;
	std::atomic<int64_t> stream_time_;
	DWORD queue_id_;
	uint32_t latency_;
	uint32_t buffer_frames_;
	std::wstring mmcss_name_;
	MmcssThreadPriority thread_priority_;
	mutable FastMutex render_mutex_;
	MFWORKITEM_KEY sample_ready_key_;
	WinHandle sample_ready_;
	CComHeapPtr<WAVEFORMATEX> mix_format_;
	CComPtr<IMMDevice> device_;
	CComPtr<IAudioClient3> client_;
	CComPtr<IAudioClock> clock_;
	CComPtr<IAudioRenderClient> render_client_;
	CComPtr<DeviceEventNotification> device_volume_notification_;
	CComPtr<MFAsyncCallback<SharedWasapiDevice>> sample_ready_callback_;
	CComPtr<IMFAsyncResult> sample_ready_async_result_;
	CComPtr<MFAsyncCallback<SharedWasapiDevice>> start_playback_callback_;
	CComPtr<IMFAsyncResult> start_playback_async_result_;
	CComPtr<MFAsyncCallback<SharedWasapiDevice>> pause_playback_callback_;
	CComPtr<IMFAsyncResult> pause_playback_async_result_;
	CComPtr<MFAsyncCallback<SharedWasapiDevice>> stop_playback_callback_;
	CComPtr<IMFAsyncResult> stop_playback_async_result_;
	CComPtr<IAudioEndpointVolume> endpoint_volume_;
	AudioCallback* callback_;
	std::shared_ptr<spdlog::logger> log_;
};

}

#endif


