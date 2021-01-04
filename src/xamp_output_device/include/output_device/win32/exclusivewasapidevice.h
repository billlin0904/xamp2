//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <mutex>
#include <atomic>

#include <base/memory.h>
#include <base/dataconverter.h>
#include <base/vmmemlock.h>

#ifdef XAMP_OS_WIN

#include <output_device/audiocallback.h>
#include <output_device/dsddevice.h>
#include <output_device/device.h>

namespace xamp::output_device::win32 {

using namespace base;

class XAMP_OUTPUT_DEVICE_API ExclusiveWasapiDevice final : public Device {
public:
	explicit ExclusiveWasapiDevice(CComPtr<IMMDevice> const & device);

	virtual ~ExclusiveWasapiDevice();

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

	void SetVolume(uint32_t volume) const override;

	void SetMute(bool mute) const override;

	void DisplayControlPanel() override;

	bool IsMuted() const override;

	PackedFormat GetPackedFormat() const noexcept override;

	void SetSchedulerService(std::wstring const & mmcss_name, MmcssThreadPriority thread_priority);

	uint32_t GetBufferSize() const noexcept override;

	bool IsHardwareControlVolume() const override;

	void AbortStream() noexcept override;
private:	

	void InitialDeviceFormat(AudioFormat const & output_format, uint32_t valid_bits_samples);	

	void SetAlignedPeriod(REFERENCE_TIME device_period, AudioFormat const & output_format);

	void FillSilentSample(uint32_t frames_available) noexcept;

	void ReportError(HRESULT hr) noexcept;

	void GetSample(uint32_t frame_available) noexcept;

	HRESULT OnSampleReady(IMFAsyncResult* result) noexcept;

	bool raw_mode_;
	std::atomic<bool> is_running_;
	std::atomic<bool> is_stop_streaming_;
	MmcssThreadPriority thread_priority_;
	uint32_t buffer_frames_;
	uint32_t valid_bits_samples_;
	DWORD volume_support_mask_;
	DWORD queue_id_;	
	std::atomic<int64_t> stream_time_;
	WinHandle sample_ready_;
	std::wstring mmcss_name_;
	AudioConvertContext data_convert_;
	MFWORKITEM_KEY sample_ready_key_;
	REFERENCE_TIME aligned_period_;
	CComPtr<IMMDevice> device_;
	CComPtr<IAudioClient2> client_;
	CComPtr<IAudioRenderClient> render_client_;
	CComPtr<IAudioEndpointVolume> endpoint_volume_;
	CComPtr<IAudioClock> clock_;
	CComHeapPtr<WAVEFORMATEX> mix_format_;
	CComPtr<MFAsyncCallback<ExclusiveWasapiDevice>> sample_ready_callback_;
	CComPtr<IMFAsyncResult> sample_ready_async_result_;	
	mutable std::mutex mutex_;
	Buffer<float> buffer_;
	std::condition_variable condition_;
	AudioCallback* callback_;
	VmMemLock vmlock_;
	std::shared_ptr<spdlog::logger> log_;
};

}

#endif
