//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#if XAMP_OS_WIN

#include <atomic>

#include <asio.h>

#include <base/logger.h>
#include <base/stl.h>
#include <base/dsdsampleformat.h>
#include <base/buffer.h>
#include <base/fastmutex.h>
#include <base/fastconditionvariable.h>
#include <output_device/ioutputdevice.h>
#include <output_device/idsddevice.h>
#include <output_device/win32/mmcss.h>

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_BEGIN

class AsioDevice final : public IOutputDevice, public IDsdDevice {
public:
	explicit AsioDevice(const std::string & device_id);

	virtual ~AsioDevice() override;

    void setBitPerfect(bool enabled) override { bitperfect_ = enabled; }
    void setIntegerPcmFormat(const xamp::pcm::Format& format) override { source_pcm_ = format; }
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

	void setVolume(uint32_t volume) const override;

	void setMute(bool mute) const override;

	PackedFormat getPackedFormat() const override;

	void setIoFormat(DsdIoFormat format) override;

	DsdIoFormat getIoFormat() const override;
	
	DsdFormat getSampleFormat() const ;

	uint32_t getBufferSize() const override;

	bool isMuted() const override;

	bool isHardwareControlVolume() const override;

	void abortStream() override;

	void reOpen();

	bool isSupportDsdFormat() const;

	static void resetCurrentDriver();

	void setSampleFormat(DsdFormat format);

	void removeCurrentDriver();

private:
	static ASIOTime* onBufferSwitchTimeInfoCallback(ASIOTime* timeInfo, long index, ASIOBool processNow) ;

	static void onBufferSwitchCallback(long index, ASIOBool processNow);

	static void onSampleRateChangedCallback(ASIOSampleRate sampleRate);

	static long onAsioMessagesCallback(long selector, long value, void* message, double* opt);

	void setOutputSampleRate(AudioFormat const & output_format);

	void createBuffers(AudioFormat const & output_format);

	void getSamples(long index, double sample_time) ;

	std::tuple<int32_t, int32_t> getDeviceBufferSize() const;

	void fillSilentData() ;

	bool getPCMSamples(long index, double sample_time, size_t& num_filled_frame) ;

	bool getDSDSamples(long index, double sample_time, size_t& num_filled_frame) ;

    bool bitperfect_{false};
    xamp::pcm::Format source_pcm_, target_pcm_;
    std::atomic<int64_t> pcm_frames_{0};
    std::atomic<bool> pcm_rate_changed_{false};
    int drain_buffers_{-1};
    int drain_callback_count_{2};
    void renderInteger(long index, double sample_time);
	bool is_hardware_control_volume_;
	bool is_removed_driver_;
	mutable std::atomic<bool> is_stopped_;
	mutable std::atomic<bool> is_streaming_;
	mutable std::atomic<bool> is_stop_streaming_;
	int64_t latency_;
	DsdIoFormat io_format_;
	DsdFormat sample_format_;
	mutable std::atomic<uint32_t> volume_level_;
	size_t buffer_size_;
	size_t buffer_bytes_;
	std::atomic<int64_t> output_bytes_;	
	mutable std::mutex mutex_;
	std::condition_variable condition_;
	AudioFormat format_;
	std::vector<ASIOClockSource> clock_source_;	
	IAudioCallback* callback_;
	std::move_only_function<bool(long, double, size_t&)> get_samples_;
	LoggerPtr logger_;
	std::string device_id_;
	Buffer<std::byte> buffer_;
};

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_END

#endif

