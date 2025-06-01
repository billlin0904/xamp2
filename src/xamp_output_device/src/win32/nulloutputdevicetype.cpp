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

	[[nodiscard]] size_t GetDeviceCount() const;

	[[nodiscard]] DeviceInfo GetDeviceInfo(uint32_t device) const;

	[[nodiscard]] std::optional<DeviceInfo> GetDefaultDeviceInfo() const;

	[[nodiscard]] std::vector<DeviceInfo> GetDeviceInfo() const;

	ScopedPtr<IOutputDevice> MakeDevice(const std::shared_ptr<IThreadPoolExecutor>& thread_pool, const std::string& device_id);
	
private:
	LoggerPtr logger_;
};

NullOutputDeviceType::NullOutputDeviceTypeImpl::NullOutputDeviceTypeImpl() noexcept {
	logger_ = XampLoggerFactory.GetLogger(kNullOutputDeviceTypeLoggerName);
}

void NullOutputDeviceType::NullOutputDeviceTypeImpl::ScanNewDevice() {
}

ScopedPtr<IOutputDevice> NullOutputDeviceType::NullOutputDeviceTypeImpl::MakeDevice(const std::shared_ptr<IThreadPoolExecutor>& thread_pool, const std::string & device_id) {
	return MakeAlign<IOutputDevice, NullOutputDevice>(thread_pool);
}

DeviceInfo NullOutputDeviceType::NullOutputDeviceTypeImpl::GetDeviceInfo(uint32_t device) const {
	return GetDeviceInfo().at(device);
}

size_t NullOutputDeviceType::NullOutputDeviceTypeImpl::GetDeviceCount() const {
	return 1;
}

std::vector<DeviceInfo> NullOutputDeviceType::NullOutputDeviceTypeImpl::GetDeviceInfo() const {
	std::vector<DeviceInfo> device_info_list;
	device_info_list.push_back(GetDefaultDeviceInfo().value());
	return device_info_list;
}

std::optional<DeviceInfo> NullOutputDeviceType::NullOutputDeviceTypeImpl::GetDefaultDeviceInfo() const {	
	DeviceInfo info;
	info.name           = kDescription;
	info.device_id      = kDeviceId;
	info.is_support_dsd = true;
	info.device_type_id = XAMP_UUID_OF(NullOutputDeviceType);
	return MakeOptional<DeviceInfo>(std::move(info));
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

std::vector<DeviceInfo> NullOutputDeviceType::GetDeviceInfo() const {
	return impl_->GetDeviceInfo();
}

ScopedPtr<IOutputDevice> NullOutputDeviceType::MakeDevice(const std::shared_ptr<IThreadPoolExecutor>& thread_pool, const std::string& device_id) {
	return impl_->MakeDevice(thread_pool, device_id);
}

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_END
