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

	size_t GetDeviceCount() const;

    DeviceInfo GetDeviceInfo(uint32_t device) const;

	std::optional<DeviceInfo> GetDefaultDeviceInfo() const;

	Vector<DeviceInfo> GetDeviceInfo() const;

    void ScanNewDevice();

	AlignPtr<IOutputDevice> MakeDevice(const  std::string &device_id);
private:
	DeviceInfo GetDeviceInfo(std::wstring const& name, const  std::string & device_id) const;

	static HashMap<std::string, DeviceInfo> device_info_cache_;
};

HashMap<std::string, DeviceInfo> AsioDeviceType::AsioDeviceTypeImpl::device_info_cache_;

DeviceInfo AsioDeviceType::AsioDeviceTypeImpl::GetDeviceInfo(uint32_t device) const {
	auto itr = device_info_cache_.begin();
	if (device >= GetDeviceCount()) {
		throw DeviceNotFoundException();
	}
	std::advance(itr, device);
	return (*itr).second;
}

std::optional<DeviceInfo> AsioDeviceType::AsioDeviceTypeImpl::GetDefaultDeviceInfo() const {
	if (device_info_cache_.empty()) {
		return std::nullopt;
	}
	return std::optional<DeviceInfo> { std::in_place_t{}, GetDeviceInfo(0) };
}

Vector<DeviceInfo> AsioDeviceType::AsioDeviceTypeImpl::GetDeviceInfo() const {
	Vector<DeviceInfo> device_infos;
	device_infos.reserve(device_info_cache_.size());

	for (const auto& device_info : device_info_cache_) {
		device_infos.push_back(device_info.second);
	}
	return device_infos;
}

void AsioDeviceType::AsioDeviceTypeImpl::ScanNewDevice() {
    constexpr auto kMaxPathLen = 256;

	AsioDrivers drivers;
	const auto num_device = drivers.asioGetNumDev();

	for (auto i = 0; i < num_device; ++i) {		
		CLSID clsid{ 0 };
		if (drivers.asioGetDriverCLSID(i, &clsid) == 0) {
			char driver_name[kMaxPathLen + 1]{};
			drivers.asioGetDriverName(i, driver_name, kMaxPathLen);
			if (!device_info_cache_.contains(driver_name)) {
				device_info_cache_[driver_name] = GetDeviceInfo(String::ToStdWString(driver_name), driver_name);
			}			
		}
	}
}

DeviceInfo AsioDeviceType::AsioDeviceTypeImpl::GetDeviceInfo(std::wstring const& name, const  std::string & device_id) const {
	DeviceInfo info;
	info.name = name;
	info.device_id = device_id;
	info.device_type_id = XAMP_UUID_OF(AsioDeviceType);
	info.is_support_dsd = true;
	info.is_hardware_control_volume = true;
	return info;
}

AlignPtr<IOutputDevice> AsioDeviceType::AsioDeviceTypeImpl::MakeDevice(const  std::string & device_id) {
	return MakeAlign<IOutputDevice, AsioDevice>(device_id);
}

XAMP_PIMPL_IMPL(AsioDeviceType)

AsioDeviceType::AsioDeviceType()
	: impl_(MakeAlign<AsioDeviceTypeImpl>()) {
}

std::string_view AsioDeviceType::GetDescription() const {
	return Description;
}

Uuid AsioDeviceType::GetTypeId() const {
	return XAMP_UUID_OF(AsioDeviceType);
}

size_t AsioDeviceType::GetDeviceCount() const {
	return impl_->GetDeviceCount();
}

DeviceInfo AsioDeviceType::GetDeviceInfo(uint32_t device) const {
	return impl_->GetDeviceInfo(device);
}

std::optional<DeviceInfo> AsioDeviceType::GetDefaultDeviceInfo() const {
	return impl_->GetDefaultDeviceInfo();
}

Vector<DeviceInfo> AsioDeviceType::GetDeviceInfo() const {
	return impl_->GetDeviceInfo();
}

void AsioDeviceType::ScanNewDevice() {
	impl_->ScanNewDevice();
}

AlignPtr<IOutputDevice> AsioDeviceType::MakeDevice(std::string const& device_id) {
	return impl_->MakeDevice(device_id);
}

size_t AsioDeviceType::AsioDeviceTypeImpl::GetDeviceCount() const {
	return device_info_cache_.size();
}

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_END

#endif // ENABLE_ASIO && XAMP_OS_WIN
