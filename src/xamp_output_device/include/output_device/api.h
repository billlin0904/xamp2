//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/align_ptr.h>
#include <base/uuid.h>
#include <output_device/output_device.h>

namespace xamp::output_device {

XAMP_OUTPUT_DEVICE_API AlignPtr<IAudioDeviceManager> MakeAudioDeviceManager();

XAMP_OUTPUT_DEVICE_API bool IsExclusiveDevice(DeviceInfo const& info) noexcept;

XAMP_OUTPUT_DEVICE_API bool IsAsioDevice(Uuid const& id) noexcept;

XAMP_OUTPUT_DEVICE_API void ResetAsioDriver();

XAMP_OUTPUT_DEVICE_API void PreventSleep(bool allow);

}