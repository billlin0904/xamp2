//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_WIN

#include <output_device/ioutputdevice.h>
#include <output_device/win32/wasapiworkqueue.h>

#include <base/logger.h>
#include <base/platfrom_handle.h>

#include <atomic>

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_BEGIN

XAMP_DECLARE_LOG_NAME(SharedWasapiDevice);

/*
 * SharedWasapiDevice is a shared mode output device.
 */
class SharedWasapiDevice final : public IOutputDevice {
public:
	/*
	* Constructor.
	* 
	* @param device IMMDevice
	*/
	SharedWasapiDevice(bool is_low_latency, const CComPtr<IMMDevice> & device);

	/*
	 * Destructor.
	 */
	virtual ~SharedWasapiDevice() override;

	/*
	* Open stream.
	* 
	* @param output_format AudioFormat
	*/
	void OpenStream(const AudioFormat & output_format) override;

	/*
	* Set audio callback.
	*
	* @param callback: audio callback
	* @return void
	*/
	void SetAudioCallback(IAudioCallback* callback) noexcept override;

	/*
	* Is stream open.
	*
	* @return bool
	*/
	bool IsStreamOpen() const noexcept override;

	/*
	* Is stream running.
	*
	* @return bool
	*/
	bool IsStreamRunning() const noexcept override;

	/*
	* Stop stream.
	*
	* @param wait_for_stop_stream: wait for stop stream
	*/
	void StopStream(bool wait_for_stop_stream = true) override;

	/*
	* Close stream.
	*/
	void CloseStream() override;

	/*
	* Start stream.
	*/
	void StartStream() override;

	/*
	* Set stream time.
	*
	* @param stream_time: stream time
	*/
	void SetStreamTime(double stream_time) noexcept override;

	/*
	* Get stream time.
	*
	* @return double
	*/
	double GetStreamTime() const noexcept override;

	/*
	* Get volume.
	*
	* @return uint32_t
	*/
	uint32_t GetVolume() const override;

	/*
	* Is muted.
	*
	* @return bool
	*/
	bool IsMuted() const override;

	/*
	* Set volume
	* @param volume: volume (1~100)
	*/
	void SetVolume(uint32_t volume) const override;

	/*
	* Set mute.
	*
	* @param mute: mute (true/false)
	*/
	void SetMute(bool mute) const override;

	/*
	* Get packed format.
	*
	* @return PackedFormat
	*/
	PackedFormat GetPackedFormat() const noexcept override;

	/*
	* Set scheduler service
	*
	* @param[in] mmcss_name: mmcss name
	* @param[in] thread_priority: thread priority
	*/
	void SetSchedulerService(const std::wstring & mmcss_name, MmcssThreadPriority thread_priority);

	/*
	* Get device buffer size.
	*
	* @return uint32_t
	*/
	uint32_t GetBufferSize() const noexcept override;

	/*
	* Is hardware control volume.
	*
	* @return bool
	*/
	bool IsHardwareControlVolume() const override;

	/*
	* Abort stream.
	*/
	void AbortStream() noexcept override;

private:
	HRESULT GetSample(bool is_silence) noexcept;

	HRESULT GetSample(uint32_t frame_available, bool is_silence) noexcept;

	void ReportError(HRESULT hr);

	void UnRegisterDeviceVolumeChange();

	void RegisterDeviceVolumeChange();

	void InitialDevice(const AudioFormat & output_format);

	void InitialDeviceFormat(const AudioFormat & output_format);

	HRESULT OnInvoke(IMFAsyncResult* async_result);

	class DeviceEventNotification;

	bool is_low_latency_{ true };
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
	CComPtr<ISimpleAudioVolume> simple_audio_volume_;
	CComPtr<IAudioEndpointVolume> endpoint_volume_;
	IAudioCallback* callback_;
	CComPtr<WasapiWorkQueue<SharedWasapiDevice>> rt_work_queue_;
	LoggerPtr logger_;
};

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_END

#endif // XAMP_OS_WIN
