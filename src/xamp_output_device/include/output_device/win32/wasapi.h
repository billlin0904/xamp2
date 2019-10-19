//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#ifdef _WIN32
#pragma comment(lib, "avrt.lib")
#pragma comment(lib, "Mfplat.lib")

#include <atlcomcli.h>
#include <mmdeviceapi.h>

#include <atlbase.h>
#include <mmdeviceapi.h>
#include <Audioclient.h>
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

std::wstring GetDeviceProperty(const PROPERTYKEY& key, CComPtr<IMMDevice>& device);

DeviceInfo GetDeviceInfo(CComPtr<IMMDevice>& device, const ID device_type_id);

}

#endif
