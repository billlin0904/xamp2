//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/exception.h>
#include <output_device/output_device.h>

namespace xamp::output_device {

using namespace base;

class XAMP_OUTPUT_DEVICE_API XAMP_NO_VTABLE AudioCallback {
public:
	virtual ~AudioCallback() = default;

    virtual int32_t OnGetSamples(void* samples, const uint32_t num_buffer_frames, const double stream_time) noexcept = 0;

	virtual void OnError(const Exception& exception) noexcept = 0;

	virtual void OnVolumeChange(float vol) noexcept = 0;
protected:
	AudioCallback() = default;
};

}
