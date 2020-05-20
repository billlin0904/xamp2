//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_WIN
#pragma comment(lib, "avrt.lib")
#pragma comment(lib, "Mfplat.lib")

#include <atlcomcli.h>
#include <mmdeviceapi.h>

#include <atlbase.h>
#include <mmdeviceapi.h>
#include <Audioclient.h>
#include <audiopolicy.h>
#include <mfapi.h>
#include <strmif.h>
#include <endpointvolume.h>
#include <functiondiscoverykeys_devpkey.h>

#include <base/windows_handle.h>
#include <base/id.h>

#include <avrt.h>
#include <Mferror.h>

#include <output_device/deviceinfo.h>
#include <output_device/win32/mfasynccallback.h>
#include <output_device/win32/mmcss_types.h>

struct IMMDevice;
struct IMFAsyncResult;
struct IAudioClient;
struct IAudioRenderClient;
struct IMMNotificationClient;
struct IMMDeviceEnumerator;
struct IMMDeviceEnumerator;

namespace xamp::output_device::win32::helper {

static constexpr int32_t kWasapiReftimesPerMillisec = 10000;
static constexpr double kWasapiReftimesPerSec = 10000000;

std::wstring GetDeviceProperty(PROPERTYKEY const& key, CComPtr<IMMDevice>& device);

DeviceInfo GetDeviceInfo(CComPtr<IMMDevice>& device, const ID device_type_id);

CComPtr<IMMDeviceEnumerator> CreateDeviceEnumerator();

XAMP_ALWAYS_INLINE constexpr double Nano100ToSeconds(REFERENCE_TIME ref) noexcept {
	//  1 nano = 0.000000001 seconds
	//100 nano = 0.0000001   seconds
	//100 nano = 0.0001   milliseconds
	constexpr double ratio = 0.0000001;
	return (static_cast<double>(ref) * ratio);
}

XAMP_ALWAYS_INLINE constexpr UINT32 ReferenceTimeToFrames(REFERENCE_TIME period, UINT32 samplerate) noexcept {
	return static_cast<UINT32>(
		1.0 * period * // hns *
		samplerate / // (frames / s) /
		1000 / // (ms / s) /
		10000 // (hns / s) /
		+ 0.5 // rounding
		);
}

XAMP_ALWAYS_INLINE constexpr REFERENCE_TIME MakeHnsPeriod(UINT32 frames, UINT32 samplerate) noexcept {
	return static_cast<REFERENCE_TIME>(10000.0 * 1000.0 / double(samplerate) * double(frames) + 0.5);
}

}

#endif
