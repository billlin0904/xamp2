//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/align_ptr.h>
#include <base/uuid.h>
#include <output_device/output_device.h>

namespace xamp::output_device {

XAMP_OUTPUT_DEVICE_API AlignPtr<IAudioDeviceManager> MakeAudioDeviceManager();

XAMP_OUTPUT_DEVICE_API bool IsExclusiveDevice(DeviceInfo const& info) noexcept;

XAMP_OUTPUT_DEVICE_API bool IsASIODevice(Uuid const& id) noexcept;

XAMP_OUTPUT_DEVICE_API void ResetASIODriver();

XAMP_OUTPUT_DEVICE_API void PreventSleep(bool allow);

}