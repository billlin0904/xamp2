//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <atomic>
#include <mutex>
#include <vector>

#include <base/align_ptr.h>

#include <output_device/audiocallback.h>
#include <output_device/device.h>
#include <output_device/dsdoutputable.h>

namespace xamp::output_device {

class XAMP_OUTPUT_DEVICE_API AsioDevice final
	: public Device
	, public DSDOutputable {
public:
	explicit AsioDevice(const std::string& device_id);

	~AsioDevice() override;

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

	InterleavedFormat GetInterleavedFormat() const override;

	bool IsSupportDSDFormat() const override;

	void SetIoFormat(AsioIoFormat format) override;

	AsioIoFormat GetIoFormat() const override;

	void SetSampleFormat(DSDSampleFormat format) override;

	DSDSampleFormat GetSampleFormat() const override;

	int32_t GetBufferSize() const override;

	bool IsMuted() const override;

private:
	static ASIOTime* OnBufferSwitchTimeInfoCallback(ASIOTime* timeInfo, long index, ASIOBool processNow);

	static void OnBufferSwitchCallback(long index, ASIOBool processNow);

	static void OnSampleRateChangedCallback(ASIOSampleRate sampleRate);

	static long OnAsioMessagesCallback(long selector, long value, void* message, double* opt);

	void ReOpen();

	void SetOutputSampleRate(const AudioFormat& output_format);

	void CreateBuffers(const AudioFormat& output_format);

	void OnBufferSwitch(long index);

	bool is_removed_driver_;
	std::atomic<bool> is_stopped_;
	std::atomic<bool> is_streaming_;
	std::atomic<bool> is_stop_streaming_;
	AsioIoFormat io_format_;
	DSDSampleFormat sample_format_;
	mutable double volume_;
	int32_t buffer_size_;
	int32_t buffer_bytes_;
	std::atomic<int64_t> played_bytes_;
	std::string device_id_;
	mutable std::mutex mutex_;
	std::condition_variable condition_;
	AudioFormat mix_format_;
	std::vector<ASIOClockSource> clock_source_;
	AlignBufferPtr<int8_t> buffer_;
	AlignBufferPtr<int8_t> device_buffer_;
	AudioCallback* callback_;
};

}

