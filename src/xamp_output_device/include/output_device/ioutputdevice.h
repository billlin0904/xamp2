//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <output_device/output_device.h>

#include <base/audioformat.h>

XAMP_OUTPUT_DEVICE_NAMESPACE_BEGIN

/*
* IOutputDevice is the interface for output device.
* 
*/
class XAMP_OUTPUT_DEVICE_API XAMP_NO_VTABLE IOutputDevice {
public:
	XAMP_BASE_CLASS(IOutputDevice)

	/*
	* Open stream.
	* 
	* @param output_format: output format
	* @return void
	*/	
    virtual void OpenStream(const AudioFormat & output_format) = 0;

	/*
	* Set audio callback.
	* 
	* @param callback: audio callback
	* @return void
	*/
	virtual void SetAudioCallback(IAudioCallback* callback) noexcept = 0;

	/*
	* Is stream open.
	* 
	* @return bool
	*/
	XAMP_NO_DISCARD virtual bool IsStreamOpen() const noexcept = 0;

	/*
	* Is stream running.
	* 
	* @return bool
	*/
	XAMP_NO_DISCARD virtual bool IsStreamRunning() const noexcept = 0;

	/*
	* Stop stream.
	* 
	* @param wait_for_stop_stream: wait for stop stream
	*/
	virtual void StopStream(bool wait_for_stop_stream = true) = 0;

	/*
	* Close stream.
	*/
	virtual void CloseStream() = 0;

	/*
	* Start stream.
	*/
	virtual void StartStream() = 0;

	/*
	* Set stream time.
	* 
	* @param stream_time: stream time
	*/
	virtual void SetStreamTime(double stream_time) noexcept = 0;

	/*
	* Get stream time.
	* 
	* @return double
	*/
	XAMP_NO_DISCARD virtual double GetStreamTime() const noexcept = 0;

	/*
	* Get volume.
	* 
	* @return uint32_t
	*/
    XAMP_NO_DISCARD virtual uint32_t GetVolume() const = 0;

	/*
	* Set volume.
	* 
	* @param volume: volume (1~100)
	*/
    virtual void SetVolume(uint32_t volume) const = 0;

	/*
	* Set mute.
	* 
	* @param mute: mute (true/false)
	*/
	virtual void SetMute(bool mute) const = 0;

	/*
	* Is muted.
	* 
	* @return bool
	*/
	XAMP_NO_DISCARD virtual bool IsMuted() const = 0;

	/*
	* Is hardware control volume.
	* 
	* @return bool
	*/
	XAMP_NO_DISCARD virtual bool IsHardwareControlVolume() const = 0;

	/*
	* Get packed format.
	* 
	* @return PackedFormat
	*/
	XAMP_NO_DISCARD virtual PackedFormat GetPackedFormat() const noexcept = 0;

	/*
	* Get device buffer size.
	* 
	* @return uint32_t
	*/
	XAMP_NO_DISCARD virtual uint32_t GetBufferSize() const noexcept = 0;

	/*
	* Abort stream.
	*/	
	virtual void AbortStream() noexcept = 0;

protected:
	/*
	* Constructor.
	*/
	IOutputDevice() = default;
};

XAMP_OUTPUT_DEVICE_NAMESPACE_END
