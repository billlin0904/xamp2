//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_WIN

#include <output_device/idevicestatelistener.h>
#include <output_device/idevicestatenotification.h>
#include <output_device/win32/unknownimpl.h>
#include <output_device/win32/wasapi.h>

namespace xamp::output_device::win32 {

class Win32DeviceStateNotification
	: public UnknownImpl<IMMNotificationClient>
	, public IDeviceStateNotification {
public:
	explicit Win32DeviceStateNotification(std::weak_ptr<IDeviceStateListener> callback);

	virtual ~Win32DeviceStateNotification() override;

	void Run() override;

	STDMETHOD(QueryInterface)(REFIID iid, void** object) override;

private:
	STDMETHOD(OnPropertyValueChanged)(LPCWSTR device_id, const PROPERTYKEY key) override;

	STDMETHOD(OnDeviceAdded)(LPCWSTR device_id) override;

	STDMETHOD(OnDeviceRemoved)(LPCWSTR device_id) override;

	STDMETHOD(OnDeviceStateChanged)(LPCWSTR device_id, DWORD new_state) override;

	STDMETHOD(OnDefaultDeviceChanged)(EDataFlow flow, ERole role, LPCWSTR new_default_device_id) override;

	std::weak_ptr<IDeviceStateListener> callback_;
	CComPtr<IMMDeviceEnumerator> enumerator_;
};

}

#endif
