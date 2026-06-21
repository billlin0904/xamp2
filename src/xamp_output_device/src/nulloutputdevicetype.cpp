#include <base/base.h>

#include <output_device/nulloutputdevice.h>
#include <output_device/nulloutputdevicetype.h>

#include <base/logger.h>

XAMP_OUTPUT_DEVICE_NAMESPACE_BEGIN

class NullOutputDeviceType::NullOutputDeviceTypeImpl final {
public:
	static constexpr std::wstring_view kDescription = L"Null Output Device";
	static constexpr std::string_view kDeviceId = "16AE95CF-6173-41DA-859D-EFF9D45CC504";

	NullOutputDeviceTypeImpl();

	void scanNewDevice();

	[[nodiscard]] size_t getDeviceCount() const;

	[[nodiscard]] DeviceInfo getDeviceInfo(uint32_t device) const;

	[[nodiscard]] std::optional<DeviceInfo> getDefaultDeviceInfo() const;

	[[nodiscard]] std::vector<DeviceInfo> getDeviceInfo() const;

	ScopedPtr<IOutputDevice> makeDevice(const std::shared_ptr<IThreadPool>& thread_pool, const std::string& device_id);

private:
	LoggerPtr logger_;
};

NullOutputDeviceType::NullOutputDeviceTypeImpl::NullOutputDeviceTypeImpl() {
	logger_ = XampLoggerFactory.getLogger(XAMP_LOG_NAME(NullOutputDeviceType));
}

void NullOutputDeviceType::NullOutputDeviceTypeImpl::scanNewDevice() {
}

ScopedPtr<IOutputDevice> NullOutputDeviceType::NullOutputDeviceTypeImpl::makeDevice(const std::shared_ptr<IThreadPool>& thread_pool, const std::string& device_id) {
	return makeAlign<IOutputDevice, NullOutputDevice>(thread_pool);
}

DeviceInfo NullOutputDeviceType::NullOutputDeviceTypeImpl::getDeviceInfo(uint32_t device) const {
	return getDeviceInfo().at(device);
}

size_t NullOutputDeviceType::NullOutputDeviceTypeImpl::getDeviceCount() const {
	return 1;
}

std::vector<DeviceInfo> NullOutputDeviceType::NullOutputDeviceTypeImpl::getDeviceInfo() const {
	std::vector<DeviceInfo> device_info_list;
	device_info_list.push_back(getDefaultDeviceInfo().value());
	return device_info_list;
}

std::optional<DeviceInfo> NullOutputDeviceType::NullOutputDeviceTypeImpl::getDefaultDeviceInfo() const {
	DeviceInfo info;
	info.name = kDescription;
	info.device_id = kDeviceId;
	info.is_support_dsd = true;
	info.device_type_id = XAMP_UUID_OF(NullOutputDeviceType);
	info.desc = NullOutputDeviceType::Description;
	return makeOptional<DeviceInfo>(std::move(info));
}

NullOutputDeviceType::NullOutputDeviceType()
	: impl_(makeAlign<NullOutputDeviceTypeImpl>()) {
}

void NullOutputDeviceType::scanNewDevice() {
	impl_->scanNewDevice();
}

size_t NullOutputDeviceType::getDeviceCount() const {
	return impl_->getDeviceCount();
}

DeviceInfo NullOutputDeviceType::getDeviceInfo(uint32_t device) const {
	return impl_->getDeviceInfo(device);
}

std::optional<DeviceInfo> NullOutputDeviceType::getDefaultDeviceInfo() const {
	return impl_->getDefaultDeviceInfo();
}

std::vector<DeviceInfo> NullOutputDeviceType::getDeviceInfo() const {
	return impl_->getDeviceInfo();
}

ScopedPtr<IOutputDevice> NullOutputDeviceType::makeDevice(const std::shared_ptr<IThreadPool>& thread_pool, const std::string& device_id) {
	return impl_->makeDevice(thread_pool, device_id);
}

XAMP_OUTPUT_DEVICE_NAMESPACE_END
