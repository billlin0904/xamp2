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

/*
* AsioDevice is the asio device.
* 
*/
class AsioDevice final : public IOutputDevice, public IDsdDevice {
public:
	/*
	* Constructor
	* 
	* @param device_id: device id
	*/
	explicit AsioDevice(const std::string & device_id);

	/*
	* Destructor
	*/
	virtual ~AsioDevice() override;

	/*
	* open stream
	*
	* @param output_format: output format
	* @return void
	*/
	void openStream(const AudioFormat & output_format) override;

	/*
	* Set audio callback
	*
	* @param callback: audio callback
	*/
	void setAudioCallback(IAudioCallback* callback) override;

	/*
	* Is stream open
	*
	* return bool
	*/
	bool isStreamOpen() const override;

	/*
	* Is stream running
	*
	* @return bool
	*/
	bool isStreamRunning() const override;

	/*
	* stop stream
	*
	* @param wait_for_stop_stream: wait for stop stream
	*/
	void stopStream(bool wait_for_stop_stream = true) override;

	/*
	* close stream
	*
	*/
	void closeStream() override;

	/*
	* start stream
	*
	*/
	void startStream() override;

	/*
	* Set stream time
	*
	* @param stream_time: stream time
	*/
	void setStreamTime(double stream_time) override;

	/*
	* Get stream time
	*
	*/
	double getStreamTime() const override;

	/*
	* Get volume
	*
	* @return uint32_t
	*/
	uint32_t getVolume() const override;

	/*
	* Set volume
	* @param volume: volume (1~100)
	*/
	void setVolume(uint32_t volume) const override;

	/*
	* Set mute
	*
	* @param mute: mute (true/false)
	*/
	void setMute(bool mute) const override;

	/*
	* Get packed format
	*
	* @return PackedFormat
	*/
	PackedFormat getPackedFormat() const override;

	/*
	* Set DSD IO format
	*
	* @param format: DSD IO format
	*/
	void setIoFormat(DsdIoFormat format) override;

	/*
	* Get DSD IO format
	*
	* @return DsdIoFormat
	*/
	DsdIoFormat getIoFormat() const override;
	
	/*
	* Get packed format
	*
	* @return PackedFormat
	*/
	DsdFormat getSampleFormat() const ;

	/*
	* Get device buffer size
	*
	* @return uint32_t
	*/
	uint32_t getBufferSize() const override;

	/*
	* Is muted
	*
	* @return bool
	*/
	bool isMuted() const override;

	/*
	* Is hardware control volume
	*
	* @return bool
	*/
	bool isHardwareControlVolume() const override;

	/*
	* Abort stream
	*
	* @return void
	*/
	void abortStream() override;

	/*
	* Reopen stream
	*/
	void reOpen();

	/*
	* Is support DSD format
	*/
	bool isSupportDsdFormat() const;

	/*
	* reset current ASIO driver	
	*/
	static void resetCurrentDriver();

	/*
	* Set DSD sample format
	* 
	* @param[in] format: DSD sample format
	*/
	void setSampleFormat(DsdFormat format);

	/*
	* Remove current ASIO driver
	*/	
	void removeCurrentDriver();

private:
	/*
	* On buffer switch time info callback
	* 
	* @param[in] timeInfo: time info
	* @param[in] index: index
	* @param[in] processNow: process now
	* @return ASIOTime*
	*/
	static ASIOTime* onBufferSwitchTimeInfoCallback(ASIOTime* timeInfo, long index, ASIOBool processNow) ;

	/*
	* On buffer switch callback
	* 
	* @param[in] index: index
	* @param[in] processNow: process now
	*/
	static void onBufferSwitchCallback(long index, ASIOBool processNow);

	/*
	* On sample rate changed callback
	* 
	* @param[in] sampleRate: sample rate
	*/
	static void onSampleRateChangedCallback(ASIOSampleRate sampleRate);

	/*
	* On asio messages callback
	* 
	* @param[in] selector: selector
	* @param[in] value: value
	* @param[in] message: message
	* @param[in] opt: opt
	*/
	static long onAsioMessagesCallback(long selector, long value, void* message, double* opt);

	/*
	* Set output sample rate
	* 
	* @param output_format: output format
	*/
	void setOutputSampleRate(AudioFormat const & output_format);

	/*
	* create buffers
	* 
	* @param[in] output_format: output format
	* 
	*/
	void createBuffers(AudioFormat const & output_format);

	/*
	* Get samples
	* 
	* @param[in] index: index
	* @param[in] sample_time: sample time
	*/
	void getSamples(long index, double sample_time) ;

	/*
	* Get device buffer size
	* 
	* @return std::tuple<int32_t, int32_t>
	*/
	std::tuple<int32_t, int32_t> getDeviceBufferSize() const;

	/*
	* Fill silent data
	*/
	void fillSilentData() ;

	/*
	* Get PCM samples
	* 
	* @param[in] index: index
	* @param[in] sample_time: sample time
	* @param[in] num_filled_frame: num filled frame
	* @return bool
	*/
	bool getPCMSamples(long index, double sample_time, size_t& num_filled_frame) ;

	/*
	* Get DSD samples
	*/
	bool getDSDSamples(long index, double sample_time, size_t& num_filled_frame) ;

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

