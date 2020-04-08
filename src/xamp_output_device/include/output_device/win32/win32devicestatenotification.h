//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_WIN

#include <output_device/devicestatelistener.h>
#include <output_device/devicestatenotification.h>
#include <output_device/win32/wasapi.h>

namespace xamp::output_device::win32 {

class XAMP_OUTPUT_DEVICE_API Win32DeviceStateNotification : public IMMNotificationClient, public DeviceStateNotification {
public:
	explicit Win32DeviceStateNotification(std::weak_ptr<DeviceStateListener> callback);

	virtual ~Win32DeviceStateNotification();

	void Run();

	STDMETHOD_(ULONG, AddRef)() override;

	STDMETHOD_(ULONG, Release)() override;

	STDMETHOD(QueryInterface)(REFIID iid, void** object) override;

private:
	STDMETHOD(OnPropertyValueChanged)(LPCWSTR device_id, const PROPERTYKEY key) override;

	STDMETHOD(OnDeviceAdded)(LPCWSTR device_id) override;

	STDMETHOD(OnDeviceRemoved)(LPCWSTR device_id) override;

	STDMETHOD(OnDeviceStateChanged)(LPCWSTR device_id, DWORD new_state) override;

	STDMETHOD(OnDefaultDeviceChanged)(EDataFlow flow, ERole role, LPCWSTR new_default_device_id) override;

	std::weak_ptr<DeviceStateListener> callback_;
	CComPtr<IMMDeviceEnumerator> enumerator_;
};

}

#endif
