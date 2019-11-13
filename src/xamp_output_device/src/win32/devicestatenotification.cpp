#ifdef _WIN32

#include <output_device/win32/hrexception.h>
#include <output_device/win32/devicestatenotification.h>

namespace xamp::output_device::win32 {

DeviceStateNotification::DeviceStateNotification(std::weak_ptr<DeviceStateListener> callback)
	: callback_(callback) {
}

DeviceStateNotification::~DeviceStateNotification() {
	enumerator_->UnregisterEndpointNotificationCallback(this);
}

void DeviceStateNotification::Run() {
	HrIfFailledThrow(CoCreateInstance(__uuidof(MMDeviceEnumerator),
		nullptr,
		CLSCTX_ALL,
		__uuidof(IMMDeviceEnumerator),
		reinterpret_cast<void**>(&enumerator_)));
	HrIfFailledThrow(enumerator_->RegisterEndpointNotificationCallback(this));
}

STDMETHODIMP_(ULONG) DeviceStateNotification::AddRef() {
	return 1;
}

STDMETHODIMP_(ULONG) DeviceStateNotification::Release() {
	return 1;
}

STDMETHODIMP DeviceStateNotification::QueryInterface(REFIID iid, void** object) {
	if (iid == IID_IUnknown || iid == __uuidof(IMMNotificationClient)) {
		*object = static_cast<IMMNotificationClient*>(this);
		return S_OK;
	}
	*object = nullptr;
	return E_NOINTERFACE;
}

STDMETHODIMP DeviceStateNotification::OnPropertyValueChanged(
	LPCWSTR device_id, const PROPERTYKEY key) {
	return S_OK;
}

STDMETHODIMP DeviceStateNotification::OnDeviceAdded(LPCWSTR device_id) {
	if (auto callback = callback_.lock()) {
		callback->OnDeviceStateChange(DeviceState::DEVICE_STATE_ADDED, device_id);
	}
	return S_OK;
}

STDMETHODIMP DeviceStateNotification::OnDeviceRemoved(LPCWSTR device_id) {
	if (auto callback = callback_.lock()) {
		callback->OnDeviceStateChange(DeviceState::DEVICE_STATE_REMOVED, device_id);
	}
	return S_OK;
}

STDMETHODIMP DeviceStateNotification::OnDeviceStateChanged(LPCWSTR device_id,
	DWORD new_state) {
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

STDMETHODIMP DeviceStateNotification::OnDefaultDeviceChanged(EDataFlow flow,
	ERole role, LPCWSTR new_default_device_id) {
	if (auto callback = callback_.lock()) {
		callback->OnDeviceStateChange(DeviceState::DEVICE_STATE_DEFAULT_DEVICE_CHANGE, new_default_device_id);
	}
	return S_OK;
}

}

#endif
