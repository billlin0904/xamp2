//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <output_device/output_device.h>

#include <base/exception.h>
#include <base/enum.h>

XAMP_OUTPUT_DEVICE_NAMESPACE_BEGIN

/*
* DataCallbackResult is the enum for data callback result.
* 
* <remarks>
* CONTINUE: Continue.
* STOP: Stop.
* </remarks>
*/
XAMP_MAKE_ENUM(DataCallbackResult,
	CONTINUE = 0, 
	STOP)

/*
* IAudioCallback is a callback interface for audio device.
*/
class XAMP_OUTPUT_DEVICE_API XAMP_NO_VTABLE IAudioCallback {
public:
	XAMP_BASE_CLASS(IAudioCallback)

	/*
	* OnGetSamples is called when audio device need more data.
	* 
	* @param[in] samples is a pointer to buffer.
	* @param[in] num_buffer_frames is a number of frames in buffer.
	* @param[in] num_filled_bytes is a number of bytes filled in buffer.
	* @param[in] stream_time is a stream time.
	* @param[in] sample_time is a sample time.
	* 
	* @return DataCallbackResult::CONTINUE if need more data, DataCallbackResult::STOP if no need more data.
	*/
    virtual DataCallbackResult OnGetSamples(void* samples, size_t num_buffer_frames, size_t & num_filled_bytes, double stream_time, double sample_time) noexcept = 0;

	/*
	* OnError is called when audio device error.
	* 
	* @param[in] exception is a exception.
	*/
    virtual void OnError(Exception const & exception) noexcept = 0;

	/*
	* OnVolumeChange is called when audio device volume changed.
	* 
	* @param[in] vol is a volume.
	*/	
	virtual void OnVolumeChange(float vol) noexcept = 0;
protected:
	/*
	* Constructor.
	*/
	IAudioCallback() = default;
};

XAMP_OUTPUT_DEVICE_NAMESPACE_END
