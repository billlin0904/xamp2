//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/exception.h>
#include <base/enum.h>
#include <output_device/output_device.h>

namespace xamp::output_device {

using namespace base;

MAKE_XAMP_ENUM(DataCallbackResult, CONTINUE = 0, STOP)

class XAMP_OUTPUT_DEVICE_API XAMP_NO_VTABLE IAudioCallback {
public:
	XAMP_BASE_CLASS(IAudioCallback)

    virtual DataCallbackResult OnGetSamples(void* samples, size_t num_buffer_frames, size_t & num_filled_bytes, double stream_time, double sample_time) noexcept = 0;

    virtual void OnError(Exception const & exception) noexcept = 0;

	virtual void OnVolumeChange(float vol) noexcept = 0;
protected:
	IAudioCallback() = default;
};

}
