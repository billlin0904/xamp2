//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/memory.h>
#include <base/uuid.h>
#include <output_device/output_device.h>

XAMP_OUTPUT_DEVICE_NAMESPACE_BEGIN

/*
* Make audio device manager.
*/
XAMP_OUTPUT_DEVICE_API ScopedPtr<IAudioDeviceManager> MakeAudioDeviceManager();

/*
* Is exclusive device.
* 
* @param[in] info: device info.
*/
XAMP_OUTPUT_DEVICE_API [[nodiscard]] bool IsExclusiveDevice(const DeviceInfo& info) noexcept;

/*
* Is asio device.
* 
* @param[in] id: device id.
*/
XAMP_OUTPUT_DEVICE_API [[nodiscard]] bool IsAsioDevice(const Uuid& id) noexcept;

/*
* Reset asio driver.
* 
*/
XAMP_OUTPUT_DEVICE_API void ResetAsioDriver();

/*
* Prevent sleep.
* 
* @param[in] allow: allow sleep.
*/
XAMP_OUTPUT_DEVICE_API void PreventSleep(bool allow);

XAMP_OUTPUT_DEVICE_NAMESPACE_END