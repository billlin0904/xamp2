//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <cstdint>
#include <base/base.h>

#ifdef XAMP_OS_WIN
#define _ATL_DEBUG_INTERFACES
#define _ATL_DEBUG_QI
#ifdef XAMP_OUTPUT_DEVICE_API_EXPORTS
#define XAMP_OUTPUT_DEVICE_API __declspec(dllexport)
#else
#define XAMP_OUTPUT_DEVICE_API __declspec(dllimport)
#endif
#else
#define XAMP_OUTPUT_DEVICE_API
#endif

namespace xamp::output_device {

using namespace base;

struct DeviceInfo;
class DeviceType;
class Device;
class DsdDevice;
class AudioCallback;
class DeviceManager;
class DeviceStateListener;
class DeviceStateNotification;

}
