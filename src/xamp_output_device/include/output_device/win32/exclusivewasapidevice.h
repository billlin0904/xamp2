//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_WIN

#include <output_device/win32/wasapi.h>
#include <output_device/idsddevice.h>
#include <output_device/ioutputdevice.h>
#include <output_device/win32/glitchdetector.h>
#include <output_device/win32/wasapiworkqueue.h>

#include <base/logger.h>
#include <base/dataconverter.h>
#include <base/buffer.h>
#include <base/platfrom_handle.h>

#include <atlbase.h>
#include <endpointvolume.h>

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
	explicit ExclusiveWasapiDevice(const CComPtr<IMMDevice>& device);

	/*
	* Destructor.
	*/
	virtual ~ExclusiveWasapiDevice() override;

	/*
	* open stream.
	*
	* @param output_format: output format
	* @return void
	*/
	void openStream(const AudioFormat & output_format) override;

	/*
	* Set audio callback.
	* 
	* @param callback: audio callback
	*/
	void setAudioCallback(IAudioCallback* callback) override;

	/*
	* Is stream open.
	* 
	* return bool
	*/	
	bool isStreamOpen() const override;

	/*
	* Is stream running.
	* 
	* @return bool
	*/
	bool isStreamRunning() const override;

	/*
	* stop stream.
	*
	* @param[in] wait_for_stop_stream: wait for stop stream
	*/
	void stopStream(bool wait_for_stop_stream = true) override;

	/*
	* close stream.
	* 
	*/
	void closeStream() override;

	/*
	* start stream.
	* 
	*/
	void startStream() override;

	/*
	* Set stream time.
	* 
	* @param stream_time: stream time
	*/
	void setStreamTime(double stream_time) override;

	/*
	* Get stream time.
	* 
	*/
	double getStreamTime() const override;

	/*
	* Get volume.
	*
	* @return uint32_t
	*/
	uint32_t getVolume() const override;

	/*
	* Set volume.
	* @param volume: volume (1~100)
	*/
	void setVolume(uint32_t volume) const override;

	/*
	* Set mute.
	*
	* @param mute: mute (true/false)
	*/
	void setMute(bool mute) const override;

	/*
	* Is muted.
	*
	* @return bool
	*/
	bool isMuted() const override;

	/*
	* Get packed format.
	*
	* @return PackedFormat
	*/
	PackedFormat getPackedFormat() const override;

	/*
	* Set scheduler service
	* 
	* @param[in] mmcss_name: mmcss name
	* @param[in] thread_priority: thread priority
	*/
	void setSchedulerService(const std::wstring & mmcss_name, MmcssThreadPriority thread_priority);

	/*
	* Get device buffer size.
	*
	* @return uint32_t
	*/
	uint32_t getBufferSize() const override;

	/*
	* Is hardware control volume.
	*
	* @return bool
	*/
	bool isHardwareControlVolume() const override;

	/*
	* Abort stream.
	*/
	void abortStream() override;

	/*
	* Set DSD IO format.
	*
	* @param[in] format: DSD IO format
	*/
	void setIoFormat(DsdIoFormat format) override;

	/*
	* Get DSD IO format.
	*
	* @return DsdIoFormat
	*/
	DsdIoFormat getIoFormat() const override;

private:
	/*
	* initial device format
	* 
	* @param[in] output_format: output format
	* @param[in] valid_bits_samples: valid bits samples	
	*/
	void initialDeviceFormat(const AudioFormat & output_format, uint32_t valid_bits_samples);

	/*
	* Set aligned period
	* 
	* @param[in] device_period: device period
	* @param[in] output_format: output format
	*/
	void setAlignedPeriod(REFERENCE_TIME device_period, const AudioFormat & output_format);

	/*
	* Report error
	* 
	* @param hr: HRESULT
	*/
	void reportError(HRESULT hr) ;

	/*
	* Get sample
	* 
	* @param is_silence: is silence
	*/
	bool getSample(bool is_silence) ;

	HRESULT onInvoke(IMFAsyncResult* async_result);

	void setVolumeLevelScalar(float level);

	[[nodiscard]] bool isBitstreamVolumeLocked() const;

	void forceBitstreamEndpointVolume() const;

	DsdIoFormat io_format_{ DsdIoFormat::IO_FORMAT_PCM };
	bool ignore_wait_slow_;
	bool is_2432_format_;	
	std::atomic<bool> is_running_;
	MmcssThreadPriority thread_priority_;
	uint32_t buffer_frames_;
	uint64_t device_frequency_;
	REFERENCE_TIME buffer_period_;
	DWORD volume_support_mask_;
	std::atomic<int64_t> stream_time_;
	WinHandle sample_ready_;
	std::wstring mmcss_name_;
	REFERENCE_TIME aligned_period_;
	CComPtr<IMMDevice> device_;
	CComPtr<IAudioClient3> client_;
	CComPtr<IAudioRenderClient> render_client_;
	CComPtr<IAudioEndpointVolume> endpoint_volume_;
	CComPtr<IAudioClock> clock_;
	CComHeapPtr<WAVEFORMATEX> mix_format_;
	Buffer<float> buffer_;
	IAudioCallback* callback_;
	CComPtr<WasapiWorkQueue<ExclusiveWasapiDevice>> rt_work_queue_;
	FastMutex mutex_;
	AudioConverter convert_;
	mutable AudioConvertContext data_convert_;
	GlitchDetector glitch_detector_;
	LoggerPtr logger_;
};

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_END

#endif
