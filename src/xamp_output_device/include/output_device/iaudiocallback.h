//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <output_device/output_device.h>

#include <base/exception.h>
#include <base/enum.h>

XAMP_OUTPUT_DEVICE_NAMESPACE_BEGIN

XAMP_MAKE_ENUM(DataCallbackResult,
	CONTINUE = 0, 
	STOP)

class XAMP_OUTPUT_DEVICE_API XAMP_NO_VTABLE IAudioCallback {
public:
	XAMP_BASE_CLASS(IAudioCallback)

    virtual DataCallbackResult OnGetSamples(void* samples,
		size_t num_buffer_frames,
		size_t & num_filled_bytes, 
		double stream_time, 
		double sample_time) = 0;

    virtual void OnError(const std::exception& exception) = 0;

	virtual void OnVolumeChange(int32_t vol) = 0;
	
	virtual void OnGlitch(std::chrono::milliseconds duration, uint32_t count) = 0;
protected:
	IAudioCallback() = default;
};

XAMP_OUTPUT_DEVICE_NAMESPACE_END
