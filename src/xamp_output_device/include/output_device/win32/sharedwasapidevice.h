//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#ifdef _WIN32

#pragma once

#include <mutex>
#include <atomic>

#include <base/vmmemlock.h>
#include <output_device/device.h>

namespace xamp::output_device::win32 {

class XAMP_OUTPUT_DEVICE_API SharedWasapiDevice final : public Device {
public:
	explicit SharedWasapiDevice(const CComPtr<IMMDevice>& device);

	virtual ~SharedWasapiDevice();

	void OpenStream(const AudioFormat& output_format) override;

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

	InterleavedFormat GetInterleavedFormat() const noexcept override;

	void SetSchedulerService(const std::wstring& mmcss_name, MmcssThreadPriority thread_priority);

	uint32_t GetBufferSize() const noexcept override;

private:
	void InitialRawMode(const AudioFormat& output_format);

	void InitialDeviceFormat(const AudioFormat& output_format);

	void GetSampleRequested(bool is_silence);

	void GetSample(uint32_t frame_available);

	void FillSilentSample(uint32_t frame_available) const;

	HRESULT OnSampleReady(IMFAsyncResult* result);

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
	MFWORKITEM_KEY sample_raedy_key_;
	WinHandle sample_ready_;
	CComHeapPtr<WAVEFORMATEX> mix_format_;
	CComPtr<IMMDevice> device_;
	CComPtr<IAudioClient3> client_;
	CComPtr<IAudioRenderClient> render_client_;
	CComPtr<MFAsyncCallback<SharedWasapiDevice>> sample_ready_callback_;
	CComPtr<IMFAsyncResult> sample_ready_async_result_;
	AudioCallback* callback_;
};

}

#endif


