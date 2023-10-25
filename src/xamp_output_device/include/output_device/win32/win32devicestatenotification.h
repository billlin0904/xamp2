//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_WIN

#include <output_device/idevicestatelistener.h>
#include <output_device/idevicestatenotification.h>
#include <output_device/win32/unknownimpl.h>
#include <output_device/win32/wasapi.h>

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_BEGIN

/*
 * Win32DeviceStateNotification is win32 device state notification.
 */
class Win32DeviceStateNotification
	: public UnknownImpl<IMMNotificationClient>
	, public IDeviceStateNotification {
public:
	/*
	 * Constructor.
	 * 
	 * @param callback: device state listener.
	 */
	explicit Win32DeviceStateNotification(const std::weak_ptr<IDeviceStateListener>& callback);

	/*
	* Destructor.
	*/
	virtual ~Win32DeviceStateNotification() override;

	/*
	* Run.
	*/
	void Run() override;

	/*
	* QueryInterface.
	* 
	* @param[in] iid: interface id.
	* @param[in] object: object.
	* 
	* @return HRESULT
	*/
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

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_END

#endif // XAMP_OS_WIN
