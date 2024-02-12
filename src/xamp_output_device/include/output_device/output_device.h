//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
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
#elif defined(XAMP_OS_MAC)
#define XAMP_OUTPUT_DEVICE_API __attribute__((visibility("default")))
#endif

namespace xamp { namespace output_device {

using namespace base;

struct DeviceInfo;
class IDeviceType;
class IOutputDevice;
class IDsdDevice;
class IAudioDeviceManager;

class IAudioCallback;
class IDeviceStateListener;
class IDeviceStateNotification;

} }

#define XAMP_OUTPUT_DEVICE_NAMESPACE_BEGIN namespace xamp { namespace output_device {
#define XAMP_OUTPUT_DEVICE_NAMESPACE_END } }

#define XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_BEGIN namespace xamp::output_device::win32 {
#define XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_END }

#define XAMP_OUTPUT_DEVICE_WIN32_HELPER_NAMESPACE_BEGIN namespace xamp::output_device::win32::helper {
#define XAMP_OUTPUT_DEVICE_WIN32_HELPER_NAMESPACE_END }
