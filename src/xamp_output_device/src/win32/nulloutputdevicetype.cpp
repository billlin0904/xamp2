#include <base/base.h>

#include <output_device/win32/nulloutputdevicetype.h>
#include <output_device/win32/nulloutputdevice.h>

#include <base/logger.h>
#include <base/logger_impl.h>

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_BEGIN

class NullOutputDeviceType::NullOutputDeviceTypeImpl final {
public:
	static constexpr std::wstring_view kDescription = L"Null Output Device";
	static constexpr std::string_view kDeviceId = "16AE95CF-6173-41DA-859D-EFF9D45CC504";

	NullOutputDeviceTypeImpl() noexcept;

	void ScanNewDevice();

	XAMP_NO_DISCARD size_t GetDeviceCount() const;

	XAMP_NO_DISCARD DeviceInfo GetDeviceInfo(uint32_t device) const;

	XAMP_NO_DISCARD std::optional<DeviceInfo> GetDefaultDeviceInfo() const;

	XAMP_NO_DISCARD Vector<DeviceInfo> GetDeviceInfo() const;

	AlignPtr<IOutputDevice> MakeDevice(const std::string& device_id);
	
private:
	LoggerPtr logger_;
};

NullOutputDeviceType::NullOutputDeviceTypeImpl::NullOutputDeviceTypeImpl() noexcept {
	logger_ = XampLoggerFactory.GetLogger(kNullOutputDeviceTypeLoggerName);
}

void NullOutputDeviceType::NullOutputDeviceTypeImpl::ScanNewDevice() {
}

AlignPtr<IOutputDevice> NullOutputDeviceType::NullOutputDeviceTypeImpl::MakeDevice(const  std::string & device_id) {
	return MakeAlign<IOutputDevice, NullOutputDevice>();
}

DeviceInfo NullOutputDeviceType::NullOutputDeviceTypeImpl::GetDeviceInfo(uint32_t device) const {
	return GetDeviceInfo().at(device);
}

size_t NullOutputDeviceType::NullOutputDeviceTypeImpl::GetDeviceCount() const {
	return 1;
}

Vector<DeviceInfo> NullOutputDeviceType::NullOutputDeviceTypeImpl::GetDeviceInfo() const {
	Vector<DeviceInfo> device_info_list;
	device_info_list.push_back(GetDefaultDeviceInfo().value());
	return device_info_list;
}

std::optional<DeviceInfo> NullOutputDeviceType::NullOutputDeviceTypeImpl::GetDefaultDeviceInfo() const {	
	DeviceInfo info;
	info.name           = kDescription;
	info.device_id      = kDeviceId;
	info.is_support_dsd = true;
	info.device_type_id = XAMP_UUID_OF(NullOutputDeviceType);
	return std::optional<DeviceInfo>{ std::in_place_t{}, info };
}

NullOutputDeviceType::NullOutputDeviceType() noexcept
	: impl_(MakeAlign<NullOutputDeviceTypeImpl>()) {
}

void NullOutputDeviceType::ScanNewDevice() {
	impl_->ScanNewDevice();
}

std::string_view NullOutputDeviceType::GetDescription() const {
	return Description;
}

Uuid NullOutputDeviceType::GetTypeId() const {
	return XAMP_UUID_OF(NullOutputDeviceType);
}

size_t NullOutputDeviceType::GetDeviceCount() const {
	return impl_->GetDeviceCount();
}

DeviceInfo NullOutputDeviceType::GetDeviceInfo(uint32_t device) const {
	return impl_->GetDeviceInfo(device);
}

std::optional<DeviceInfo> NullOutputDeviceType::GetDefaultDeviceInfo() const {
	return impl_->GetDefaultDeviceInfo();
}

Vector<DeviceInfo> NullOutputDeviceType::GetDeviceInfo() const {
	return impl_->GetDeviceInfo();
}

AlignPtr<IOutputDevice> NullOutputDeviceType::MakeDevice(const std::string& device_id) {
	return impl_->MakeDevice(device_id);
}

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_END
