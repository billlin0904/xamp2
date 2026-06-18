#include <output_device/win32/asiodevicetype.h>

#if defined(XAMP_OS_WIN)

#include <output_device/win32/asiodevice.h>

#include <base/memory.h>
#include <base/str_utilts.h>

#include <asiodrivers.h>

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_BEGIN

class AsioDeviceType::AsioDeviceTypeImpl final {
public:
	AsioDeviceTypeImpl() = default;

	size_t getDeviceCount() const;

    DeviceInfo getDeviceInfo(uint32_t device) const;

	std::optional<DeviceInfo> getDefaultDeviceInfo() const;

	std::vector<DeviceInfo> getDeviceInfo() const;

    void scanNewDevice();

	ScopedPtr<IOutputDevice> makeDevice(const  std::string &device_id);
private:
	DeviceInfo getDeviceInfo(std::wstring const& name, const  std::string & device_id) const;

	HashMap<std::string, DeviceInfo> device_info_cache_;
};

DeviceInfo AsioDeviceType::AsioDeviceTypeImpl::getDeviceInfo(uint32_t device) const {
	auto itr = device_info_cache_.begin();
	if (device >= getDeviceCount()) {
		throw DeviceNotFoundException();
	}
	std::advance(itr, device);
	return (*itr).second;
}

std::optional<DeviceInfo> AsioDeviceType::AsioDeviceTypeImpl::getDefaultDeviceInfo() const {
	if (device_info_cache_.empty()) {
		return std::nullopt;
	}
	return MakeOptional<DeviceInfo>(getDeviceInfo(0));
}

std::vector<DeviceInfo> AsioDeviceType::AsioDeviceTypeImpl::getDeviceInfo() const {
	std::vector<DeviceInfo> device_infos;
	device_infos.reserve(device_info_cache_.size());

	for (const auto& device_info : device_info_cache_) {
		device_infos.push_back(device_info.second);
	}
	return device_infos;
}

void AsioDeviceType::AsioDeviceTypeImpl::scanNewDevice() {
    constexpr auto kMaxPathLen = 256;

	AsioDrivers drivers;
	const auto num_device = drivers.asioGetNumDev();

	for (auto i = 0; i < num_device; ++i) {		
		CLSID clsid{ 0 };
		if (drivers.asioGetDriverCLSID(i, &clsid) == 0) {
			char driver_name[kMaxPathLen + 1]{};
			drivers.asioGetDriverName(i, driver_name, kMaxPathLen);
			if (!device_info_cache_.contains(driver_name)) {
				device_info_cache_[driver_name] = getDeviceInfo(String::toStdWString(driver_name), driver_name);
			}			
		}
	}
}

DeviceInfo AsioDeviceType::AsioDeviceTypeImpl::getDeviceInfo(std::wstring const& name, const  std::string & device_id) const {
	DeviceInfo info;
	info.name = name;
	info.device_id = device_id;
	info.device_type_id = XAMP_UUID_OF(AsioDeviceType);
	info.is_support_dsd = true;
	info.is_hardware_control_volume = false;
	info.desc = AsioDeviceType::Description;
	return info;
}

ScopedPtr<IOutputDevice> AsioDeviceType::AsioDeviceTypeImpl::makeDevice(const  std::string & device_id) {
	return makeAlign<IOutputDevice, AsioDevice>(device_id);
}

XAMP_PIMPL_IMPL(AsioDeviceType)

AsioDeviceType::AsioDeviceType()
	: impl_(makeAlign<AsioDeviceTypeImpl>()) {
}

size_t AsioDeviceType::getDeviceCount() const {
	return impl_->getDeviceCount();
}

DeviceInfo AsioDeviceType::getDeviceInfo(uint32_t device) const {
	return impl_->getDeviceInfo(device);
}

std::optional<DeviceInfo> AsioDeviceType::getDefaultDeviceInfo() const {
	return impl_->getDefaultDeviceInfo();
}

std::vector<DeviceInfo> AsioDeviceType::getDeviceInfo() const {
	return impl_->getDeviceInfo();
}

void AsioDeviceType::scanNewDevice() {
	impl_->scanNewDevice();
}

ScopedPtr<IOutputDevice> AsioDeviceType::makeDevice(const std::shared_ptr<IThreadPool>&, std::string const& device_id) {
	return impl_->makeDevice(device_id);
}

size_t AsioDeviceType::AsioDeviceTypeImpl::getDeviceCount() const {
	return device_info_cache_.size();
}

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_END

#endif
