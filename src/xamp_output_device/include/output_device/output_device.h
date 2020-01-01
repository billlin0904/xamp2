//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#ifdef _WIN32
#ifdef XAMP_OUTPUT_DEVICE_API_EXPORTS
#define XAMP_OUTPUT_DEVICE_API __declspec(dllexport)
#else
#define XAMP_OUTPUT_DEVICE_API __declspec(dllimport)
#endif
#else
#define XAMP_OUTPUT_DEVICE_API
#endif

#include <cstdint>

namespace xamp::output_device {

using namespace base;

class DeviceType;
class Device;
class AudioCallback;
class DeviceFactory;

XAMP_OUTPUT_DEVICE_API void InitialDevice();
XAMP_OUTPUT_DEVICE_API void UnInitialDevice();

}
