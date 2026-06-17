//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/buffer.h>
#include <base/fastconditionvariable.h>
#include <base/threadpool.h>
#include <base/logger.h>
#include <base/task.h>

#include <output_device/idsddevice.h>
#include <output_device/ioutputdevice.h>

XAMP_OUTPUT_DEVICE_NAMESPACE_BEGIN

XAMP_DECLARE_LOG_NAME(NullOutputDevice);

/*
* NullOutputDevice is the null output device.
*
*/
class NullOutputDevice final : public IOutputDevice, public IDsdDevice {
public:
	/*
	* Constructor
	*/
	explicit NullOutputDevice(const std::shared_ptr<IThreadPool>& thread_pool);

	/*
	* Destructor
	*/
	virtual ~NullOutputDevice() override;

	/*
	* open stream
	*
	* @param output_format: output format
	* @return void
	*/
	void openStream(AudioFormat const& output_format) override;

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
	* @param[in] wait_for_stop_stream: wait for stop stream
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
	* Is muted
	*
	* @return bool
	*/
	bool isMuted() const override;

	/*
	* Set volume
	* @param volume: volume (1~100)
	*/
	void setVolume(uint32_t volume) const override;

	/*
	* Set mute
	*
	* @param[in] mute: mute (true/false)
	*/
	void setMute(bool mute) const override;

	/*
	* Get packed format
	*
	* @return PackedFormat
	*/
	PackedFormat getPackedFormat() const override;

	/*
	* Get device buffer size
	*
	* @return uint32_t
	*/
	uint32_t getBufferSize() const override;

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
	[[nodiscard]] virtual DsdIoFormat getIoFormat() const override;

private:
	bool is_running_;
	bool raw_mode_;
	bool is_playing_;
	mutable bool is_muted_;
	std::atomic<bool> is_stopped_;
	mutable uint32_t volume_;
	uint32_t buffer_frames_;
	std::atomic<int64_t> stream_time_;
	IAudioCallback* callback_;
	Future<void> render_task_;
	std::chrono::milliseconds wait_time_;
	LoggerPtr logger_;
	AudioFormat output_format_;
	Buffer<float> buffer_;
	std::shared_ptr<IThreadPool> thread_pool_;
};

XAMP_OUTPUT_DEVICE_NAMESPACE_END
