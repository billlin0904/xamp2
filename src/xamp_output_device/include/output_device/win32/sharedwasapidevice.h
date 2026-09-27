//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_WIN

#include <output_device/idsddevice.h>
#include <output_device/ioutputdevice.h>
#include <output_device/win32/mmcss_types.h>
#include <output_device/win32/wasapi.h>
#include <output_device/win32/wasapiworkqueue.h>

#include <base/logger.h>
#include <base/platfrom_handle.h>
#include <base/fastconditionvariable.h>

#include <atlbase.h>
#include <endpointvolume.h>

#include <atomic>

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_BEGIN

XAMP_DECLARE_LOG_NAME(SharedWasapiDevice);

class SharedWasapiDevice final : public IOutputDevice, public IDsdDevice {
public:
	SharedWasapiDevice(bool is_low_latency, const CComPtr<IMMDevice> & device);

	virtual ~SharedWasapiDevice() override;

	void openStream(const AudioFormat & output_format) override;

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

	void setIoFormat(DsdIoFormat format) override;

	[[nodiscard]] DsdIoFormat getIoFormat() const override;

	PackedFormat getPackedFormat() const override;

	void setSchedulerService(const std::wstring & mmcss_name, MmcssThreadPriority thread_priority);

	uint32_t getBufferSize() const override;

	bool isHardwareControlVolume() const override;

	void abortStream() override;

private:
	HRESULT getSample(bool is_silence);

	HRESULT getSample(uint32_t frame_available, bool is_silence);

	void reportError(HRESULT hr);

	void unRegisterDeviceVolumeChange();

	void registerDeviceVolumeChange();

	void initialDevice(const AudioFormat & output_format);

	void initialDeviceFormat(const AudioFormat & output_format);

	[[nodiscard]] bool isBitstreamVolumeLocked() const;

	void forceBitstreamSessionVolume() const;

	HRESULT onInvoke(IMFAsyncResult* async_result);

	class DeviceEventNotification;

	bool is_low_latency_{ true };
	bool is_playing_{ false };
	bool raw_mode_{ false };
	std::atomic<bool> is_running_;
	std::atomic<int64_t> stream_time_;	
	uint32_t buffer_frames_;
	uint32_t buffer_period_in_frames_;
	REFERENCE_TIME buffer_duration_hns_;
	MmcssThreadPriority thread_priority_;
	WinHandle sample_ready_;
	CComHeapPtr<WAVEFORMATEX> mix_format_;
	CComPtr<IMMDevice> device_;
	CComPtr<IAudioClient3> client_;
	CComPtr<IAudioClock> clock_;
	CComPtr<IAudioRenderClient> render_client_;
	CComPtr<DeviceEventNotification> device_volume_notification_;
	CComPtr<ISimpleAudioVolume> simple_audio_volume_;
	CComPtr<IAudioEndpointVolume> endpoint_volume_;
	IAudioCallback* callback_;
	FastConditionVariable wait_for_start_stream_cond_;
	CComPtr<WasapiWorkQueue<SharedWasapiDevice>> rt_work_queue_;
	std::wstring mmcss_name_;
	FastMutex mutex_;
	LoggerPtr logger_;
};

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_END

#endif // XAMP_OS_WIN
