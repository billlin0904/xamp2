//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_WIN

#include <output_device/win32/wasapi.h>
#include <output_device/idsddevice.h>
#include <output_device/ioutputdevice.h>

#include <base/logger.h>
#include <base/dataconverter.h>
#include <base/buffer.h>
#include <base/task.h>
#include <base/platfrom_handle.h>

#include <atomic>

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_BEGIN

/*
* ExclusiveWasapiDevice is the exclusive mode wasapi device.
* 
*/
class ExclusiveWasapiDevice final : public IOutputDevice, public IDsdDevice {
public:
	/*
	* Constructor.
	* 
	* @param device: device	 
	*/
	explicit ExclusiveWasapiDevice(const CComPtr<IMMDevice> & device);

	/*
	* Destructor.
	*/
	virtual ~ExclusiveWasapiDevice() override;

	/*
	* Open stream.
	*
	* @param output_format: output format
	* @return void
	*/
	void OpenStream(const AudioFormat & output_format) override;

	/*
	* Set audio callback.
	* 
	* @param callback: audio callback
	*/
	void SetAudioCallback(IAudioCallback* callback) noexcept override;

	/*
	* Is stream open.
	* 
	* return bool
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
	* @param[in] wait_for_stop_stream: wait for stop stream
	*/
	void StopStream(bool wait_for_stop_stream = true) override;

	/*
	* Close stream.
	* 
	*/
	void CloseStream() override;

	/*
	* Start stream.
	* 
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
	*/
	double GetStreamTime() const noexcept override;

	/*
	* Get volume.
	*
	* @return uint32_t
	*/
	uint32_t GetVolume() const override;

	/*
	* Set volume.
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
	* Is muted.
	*
	* @return bool
	*/
	bool IsMuted() const override;

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

	/*
	* Set DSD IO format.
	*
	* @param[in] format: DSD IO format
	*/
	void SetIoFormat(DsdIoFormat format) override;

	/*
	* Get DSD IO format.
	*
	* @return DsdIoFormat
	*/
	DsdIoFormat GetIoFormat() const override;
private:
	/*
	* Initial device format
	* 
	* @param[in] output_format: output format
	* @param[in] valid_bits_samples: valid bits samples	
	*/
	void InitialDeviceFormat(const AudioFormat & output_format, uint32_t valid_bits_samples);

	/*
	* Set aligned period
	* 
	* @param[in] device_period: device period
	* @param[in] output_format: output format
	*/
	void SetAlignedPeriod(REFERENCE_TIME device_period, const AudioFormat & output_format);

	/*
	* Report error
	* 
	* @param hr: HRESULT
	*/
	void ReportError(HRESULT hr) noexcept;

	/*
	* Get sample
	* 
	* @param is_silence: is silence
	*/
	bool GetSample(bool is_silence) noexcept;
	
	bool raw_mode_;
	bool ignore_wait_slow_;
	bool is_2432_format_;
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
	LoggerPtr logger_;
	Task<void> render_task_;
};

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_END

#endif // XAMP_OS_WIN
