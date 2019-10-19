//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <mutex>
#include <atomic>

#include <base/memory.h>
#include <base/dataconverter.h>
#include <output_device/audiocallback.h>
#include <output_device/device.h>

#ifdef _WIN32

namespace xamp::output_device::win32 {

using namespace base;

class XAMP_OUTPUT_DEVICE_API ExclusiveWasapiDevice final : public Device {
public:
	explicit ExclusiveWasapiDevice(const CComPtr<IMMDevice> & device);

	virtual ~ExclusiveWasapiDevice();

	void OpenStream(const AudioFormat& output_format) override;

	void SetAudioCallback(AudioCallback* callback) override;

	bool IsStreamOpen() const override;

	bool IsStreamRunning() const override;

	void StopStream() override;

	void CloseStream() override;

	void StartStream() override;

	void SetStreamTime(double stream_time) override;

	double GetStreamTime() const override;

	int32_t GetVolume() const override;

	void SetVolume(int32_t volume) const override;

	void SetMute(bool mute) const override;

	void DisplayControlPanel() override;

	bool IsMuted() const override;

	InterleavedFormat GetInterleavedFormat() const override;

	void SetSchedulerService(const std::wstring& mmcss_name, MmcssThreadPriority thread_priority);

	int32_t GetBufferSize() const override;

private:
	void InitialDeviceFormat(const AudioFormat& output_format, int32_t valid_bits_samples);

	void FillSilentSample(int32_t frames_available) const;

	void SetAlignedPeriod(REFERENCE_TIME device_period, const AudioFormat& output_format);

	HRESULT OnSampleReady(IMFAsyncResult* result);

	std::atomic<bool> is_running_;
	std::atomic<bool> is_stop_streaming_;
	MmcssThreadPriority thread_priority_;
	int32_t frames_per_latency_;
	int32_t valid_bits_samples_;
	DWORD queue_id_;
	double stream_time_;
	WinHandle sample_ready_;
	std::wstring mmcss_name_;
	ConvertContext data_convert_;
	MFWORKITEM_KEY sample_raedy_key_;
	REFERENCE_TIME aligned_period_;
	CComPtr<IMMDevice> device_;
	CComPtr<IAudioClient2> client_;
	CComPtr<IAudioRenderClient> render_client_;
	CComPtr<IAudioEndpointVolume> endpoint_volume_;
	CComHeapPtr<WAVEFORMATEX> mix_format_;
	CComPtr<MFAsyncCallback<ExclusiveWasapiDevice>> sample_ready_callback_;
	CComPtr<IMFAsyncResult> sample_ready_async_result_;
	mutable std::mutex mutex_;
	AlignBufferPtr<float> buffer_;
	std::condition_variable condition_;
	AudioCallback* callback_;
};

}

#endif
