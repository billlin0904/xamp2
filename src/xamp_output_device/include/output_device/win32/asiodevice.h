//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#if ENABLE_ASIO && XAMP_OS_WIN

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
	* Open stream
	*
	* @param output_format: output format
	* @return void
	*/
	void OpenStream(const AudioFormat & output_format) override;

	/*
	* Set audio callback
	*
	* @param callback: audio callback
	*/
	void SetAudioCallback(IAudioCallback* callback) noexcept override;

	/*
	* Is stream open
	*
	* return bool
	*/
	bool IsStreamOpen() const noexcept override;

	/*
	* Is stream running
	*
	* @return bool
	*/
	bool IsStreamRunning() const noexcept override;

	/*
	* Stop stream
	*
	* @param wait_for_stop_stream: wait for stop stream
	*/
	void StopStream(bool wait_for_stop_stream = true) override;

	/*
	* Close stream
	*
	*/
	void CloseStream() override;

	/*
	* Start stream
	*
	*/
	void StartStream() override;

	/*
	* Set stream time
	*
	* @param stream_time: stream time
	*/
	void SetStreamTime(double stream_time) noexcept override;

	/*
	* Get stream time
	*
	*/
	double GetStreamTime() const noexcept override;

	/*
	* Get volume
	*
	* @return uint32_t
	*/
	uint32_t GetVolume() const override;

	/*
	* Set volume
	* @param volume: volume (1~100)
	*/
	void SetVolume(uint32_t volume) const override;

	/*
	* Set mute
	*
	* @param mute: mute (true/false)
	*/
	void SetMute(bool mute) const override;

	/*
	* Get packed format
	*
	* @return PackedFormat
	*/
	PackedFormat GetPackedFormat() const noexcept override;

	/*
	* Set DSD IO format
	*
	* @param format: DSD IO format
	*/
	void SetIoFormat(DsdIoFormat format) override;

	/*
	* Get DSD IO format
	*
	* @return DsdIoFormat
	*/
	DsdIoFormat GetIoFormat() const override;
	
	/*
	* Get packed format
	*
	* @return PackedFormat
	*/
	DsdFormat GetSampleFormat() const noexcept;

	/*
	* Get device buffer size
	*
	* @return uint32_t
	*/
	uint32_t GetBufferSize() const noexcept override;

	/*
	* Is muted
	*
	* @return bool
	*/
	bool IsMuted() const override;

	/*
	* Is hardware control volume
	*
	* @return bool
	*/
	bool IsHardwareControlVolume() const override;

	/*
	* Abort stream
	*
	* @return void
	*/
	void AbortStream() noexcept override;

	/*
	* Reopen stream
	*/
	void ReOpen();

	/*
	* Is support DSD format
	*/
	bool IsSupportDsdFormat() const;

	/*
	* Reset current ASIO driver	
	*/
	static void ResetCurrentDriver();

	/*
	* Set DSD sample format
	* 
	* @param[in] format: DSD sample format
	*/
	void SetSampleFormat(DsdFormat format);

	/*
	* Remove current ASIO driver
	*/
	void RemoveCurrentDriver();
private:
	/*
	* On buffer switch time info callback
	* 
	* @param[in] timeInfo: time info
	* @param[in] index: index
	* @param[in] processNow: process now
	* @return ASIOTime*
	*/
	static ASIOTime* OnBufferSwitchTimeInfoCallback(ASIOTime* timeInfo, long index, ASIOBool processNow) noexcept;

	/*
	* On buffer switch callback
	* 
	* @param[in] index: index
	* @param[in] processNow: process now
	*/
	static void OnBufferSwitchCallback(long index, ASIOBool processNow);

	/*
	* On sample rate changed callback
	* 
	* @param[in] sampleRate: sample rate
	*/
	static void OnSampleRateChangedCallback(ASIOSampleRate sampleRate);

	/*
	* On asio messages callback
	* 
	* @param[in] selector: selector
	* @param[in] value: value
	* @param[in] message: message
	* @param[in] opt: opt
	*/
	static long OnAsioMessagesCallback(long selector, long value, void* message, double* opt);

	/*
	* Set output sample rate
	* 
	* @param output_format: output format
	*/
	void SetOutputSampleRate(AudioFormat const & output_format);

	/*
	* Create buffers
	* 
	* @param[in] output_format: output format
	* 
	*/
	void CreateBuffers(AudioFormat const & output_format);

	/*
	* Get samples
	* 
	* @param[in] index: index
	* @param[in] sample_time: sample time
	*/
	void GetSamples(long index, double sample_time) noexcept;

	/*
	* Get device buffer size
	* 
	* @return std::tuple<int32_t, int32_t>
	*/
	std::tuple<int32_t, int32_t> GetDeviceBufferSize() const;

	/*
	* Fill silent data
	*/
	void FillSilentData() noexcept;

	/*
	* Get PCM samples
	* 
	* @param[in] index: index
	* @param[in] sample_time: sample time
	* @param[in] num_filled_frame: num filled frame
	* @return bool
	*/
	bool GetPCMSamples(long index, double sample_time, size_t& num_filled_frame) noexcept;

	/*
	* Get DSD samples
	*/
	bool GetDSDSamples(long index, double sample_time, size_t& num_filled_frame) noexcept;

	/*
	* Process dispatch callback	
	* 
	* @param[in] index: index
	* @param[in] sample_time: sample time
	* @param[in] num_filled_frame: num filled frame
	* @return bool
	*/
	typedef bool (AsioDevice::* ProcessDispatch)(long index, double sample_time, size_t& num_filled_frame) noexcept;

	bool is_hardware_control_volume_;
	bool is_removed_driver_;
	std::atomic<bool> is_stopped_;
	std::atomic<bool> is_streaming_;
	std::atomic<bool> is_stop_streaming_;
	long latency_;
	DsdIoFormat io_format_;
	DsdFormat sample_format_;
	mutable std::atomic<uint32_t> volume_;
	size_t buffer_size_;
	size_t buffer_bytes_;
	std::atomic<int64_t> output_bytes_;
	std::string device_id_;
	mutable FastMutex mutex_;
	FastConditionVariable condition_;
	AudioFormat format_;
	Vector<ASIOClockSource> clock_source_;
	Buffer<int8_t> buffer_;
	Buffer<int8_t> device_buffer_;	
	IAudioCallback* callback_;
	win32::Mmcss mmcss_;
	ProcessDispatch process_;
	LoggerPtr logger_;
};

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_END

#endif // ENABLE_ASIO && XAMP_OS_WIN

