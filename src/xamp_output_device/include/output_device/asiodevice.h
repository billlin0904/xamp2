//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#if ENABLE_ASIO
#include <atomic>
#include <mutex>
#include <vector>

#include <base/align_ptr.h>
#include <base/vmmemlock.h>

#include <output_device/audiocallback.h>
#include <output_device/device.h>
#include <output_device/dsddevice.h>

namespace xamp::output_device {

class XAMP_OUTPUT_DEVICE_API AsioDevice final
	: public Device
	, public DsdDevice {
public:
	explicit AsioDevice(const std::string& device_id);

	~AsioDevice() override;

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

	void SetVolume(uint32_t volume) const override;

	void SetMute(bool mute) const override;

	void DisplayControlPanel() override;

	InterleavedFormat GetInterleavedFormat() const noexcept override;

	bool IsSupportDsdFormat() const override;

	void SetIoFormat(AsioIoFormat format) override;

	AsioIoFormat GetIoFormat() const override;

	void SetSampleFormat(DsdFormat format) override;

	DsdFormat GetSampleFormat() const noexcept override;

	uint32_t GetBufferSize() const noexcept override;

	bool IsMuted() const override;

	bool CanHardwareControlVolume() const override;

private:
	static ASIOTime* OnBufferSwitchTimeInfoCallback(ASIOTime* timeInfo, long index, ASIOBool processNow) noexcept;

	static void OnBufferSwitchCallback(long index, ASIOBool processNow);

	static void OnSampleRateChangedCallback(ASIOSampleRate sampleRate);

	static long OnAsioMessagesCallback(long selector, long value, void* message, double* opt);

	void ReOpen();

	void SetOutputSampleRate(const AudioFormat& output_format);

	void CreateBuffers(const AudioFormat& output_format);

	void OnBufferSwitch(long index) noexcept;

	std::tuple<int32_t, int32_t> GetDeviceBufferSize() const;

	bool is_removed_driver_;
	std::atomic<bool> is_stopped_;
	std::atomic<bool> is_streaming_;
	std::atomic<bool> is_stop_streaming_;	
	AsioIoFormat io_format_;
	DsdFormat sample_format_;
	mutable std::atomic<int32_t> volume_;
	uint32_t buffer_size_;
	uint32_t buffer_bytes_;
	std::atomic<int64_t> played_bytes_;
	std::string device_id_;
	mutable std::mutex mutex_;
	std::condition_variable condition_;
	AudioFormat mix_format_;
	std::vector<ASIOClockSource> clock_source_;
	AlignBufferPtr<int8_t> buffer_;
	AlignBufferPtr<int8_t> device_buffer_;
	AudioCallback* callback_;
	VmMemLock buffer_vmlock_;
	VmMemLock device_buffer_vmlock_;
};

}
#endif

