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

class ExclusiveWasapiDevice final : public IOutputDevice, public IDsdDevice {
public:
	explicit ExclusiveWasapiDevice(const CComPtr<IMMDevice>& device);

	virtual ~ExclusiveWasapiDevice() override;

	void openStream(const AudioFormat & output_format) override;

	void setAudioCallback(IAudioCallback* callback) override;
	
	bool isStreamOpen() const override;

	bool isStreamRunning() const override;

	void stopStream(bool wait_for_stop_stream = true) override;

	void closeStream() override;

	void startStream() override;
    void setBitPerfect(bool enabled) override { bitperfect_ = enabled; }
    void setIntegerPcmFormat(const xamp::pcm::Format& format) override { source_pcm_ = format; }

	void setStreamTime(double stream_time) override;

	double getStreamTime() const override;

	uint32_t getVolume() const override;

	void setVolume(uint32_t volume) const override;

	void setMute(bool mute) const override;

	bool isMuted() const override;

	PackedFormat getPackedFormat() const override;

	void setSchedulerService(const std::wstring & mmcss_name, MmcssThreadPriority thread_priority);

	uint32_t getBufferSize() const override;

	bool isHardwareControlVolume() const override;

	void abortStream() override;

	void setIoFormat(DsdIoFormat format) override;

	DsdIoFormat getIoFormat() const override;

private:
	bool bitperfect_{false};
    uint32_t bitperfect_bits_{0};
    xamp::pcm::Format source_pcm_;
    Buffer<std::byte> pcm_buffer_;
	bool primed_{false};

	void initialDeviceFormat(const AudioFormat & output_format, uint32_t valid_bits_samples);

	void setAlignedPeriod(REFERENCE_TIME device_period, const AudioFormat & output_format);

	void reportError(HRESULT hr) ;

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
