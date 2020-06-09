// This is an open source non-commercial project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include <base/base.h>

#ifdef XAMP_OS_WIN

#include <output_device/win32/hrexception.h>
#include <output_device/win32/win32devicestatenotification.h>

namespace xamp::output_device::win32 {

Win32DeviceStateNotification::Win32DeviceStateNotification(std::weak_ptr<DeviceStateListener> callback)
	: callback_(callback) {
}

Win32DeviceStateNotification::~Win32DeviceStateNotification() {
	if (enumerator_ != nullptr) {
		enumerator_->UnregisterEndpointNotificationCallback(this);
	}	
}

void Win32DeviceStateNotification::Run() {
	HrIfFailledThrow(::CoCreateInstance(__uuidof(MMDeviceEnumerator),
		nullptr,
		CLSCTX_ALL,
		__uuidof(IMMDeviceEnumerator),
		reinterpret_cast<void**>(&enumerator_)));
	HrIfFailledThrow(enumerator_->RegisterEndpointNotificationCallback(this));	
}

STDMETHODIMP_(ULONG) Win32DeviceStateNotification::AddRef() {
	return 1;
}

STDMETHODIMP_(ULONG) Win32DeviceStateNotification::Release() {
	return 1;
}

STDMETHODIMP Win32DeviceStateNotification::QueryInterface(REFIID iid, void** object) {
	if (iid == IID_IUnknown || iid == __uuidof(IMMNotificationClient)) {
		*object = static_cast<IMMNotificationClient*>(this);
		return S_OK;
	}
	*object = nullptr;
	return E_NOINTERFACE;
}

STDMETHODIMP Win32DeviceStateNotification::OnPropertyValueChanged(LPCWSTR device_id, const PROPERTYKEY key) {
	return S_OK;
}

STDMETHODIMP Win32DeviceStateNotification::OnDeviceAdded(LPCWSTR device_id) {
	if (auto callback = callback_.lock()) {
		callback->OnDeviceStateChange(DeviceState::DEVICE_STATE_ADDED, device_id);
	}
	return S_OK;
}

STDMETHODIMP Win32DeviceStateNotification::OnDeviceRemoved(LPCWSTR device_id) {
	if (auto callback = callback_.lock()) {
		callback->OnDeviceStateChange(DeviceState::DEVICE_STATE_REMOVED, device_id);
	}
	return S_OK;
}

STDMETHODIMP Win32DeviceStateNotification::OnDeviceStateChanged(LPCWSTR device_id, DWORD new_state) {
	DeviceState state;

	switch (new_state) {
	case DEVICE_STATE_ACTIVE:
		state = DeviceState::DEVICE_STATE_ADDED;
		break;
	case DEVICE_STATE_NOTPRESENT:
		state = DeviceState::DEVICE_STATE_REMOVED;
		break;
	default:
		return S_OK;
	}

	if (auto callback = callback_.lock()) {
		callback->OnDeviceStateChange(state, device_id);
	}
	return S_OK;
}

STDMETHODIMP Win32DeviceStateNotification::OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR new_default_device_id) {
	if (auto callback = callback_.lock()) {
		callback->OnDeviceStateChange(DeviceState::DEVICE_STATE_DEFAULT_DEVICE_CHANGE, new_default_device_id);
	}
	return S_OK;
}

}

#endif
