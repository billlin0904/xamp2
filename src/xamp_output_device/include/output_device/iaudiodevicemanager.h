//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/stl.h>
#include <output_device/output_device.h>
#include <output_device/idevicetype.h>
#include <output_device/idevicestatelistener.h>

namespace xamp::output_device {

using DeviceTypeFactoryMap = HashMap<Uuid, std::function<AlignPtr<IDeviceType>()>>;

class XAMP_OUTPUT_DEVICE_API XAMP_NO_VTABLE IAudioDeviceManager {
public:
	XAMP_BASE_CLASS(IAudioDeviceManager)

	virtual void RegisterDeviceListener(std::weak_ptr<IDeviceStateListener> const& callback) = 0;

	[[nodiscard]] virtual AlignPtr<IDeviceType> CreateDefaultDeviceType() const = 0;

	[[nodiscard]] virtual AlignPtr<IDeviceType> Create(Uuid const& id) const = 0;

	[[nodiscard]] virtual std::vector<Uuid> GetAvailableDeviceType() const = 0;

	virtual void Clear() = 0;

	virtual DeviceTypeFactoryMap::iterator Begin() = 0;

	virtual DeviceTypeFactoryMap::iterator End() = 0;

protected:
	IAudioDeviceManager() = default;
};

}
