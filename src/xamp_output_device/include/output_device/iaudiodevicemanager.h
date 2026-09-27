//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <output_device/output_device.h>
#include <output_device/idevicetype.h>
#include <output_device/idevicestatelistener.h>

#include <base/stl.h>

#include <map>

XAMP_OUTPUT_DEVICE_NAMESPACE_BEGIN

using DeviceTypeFactoryMap = OrderedMap<Uuid, std::function<ScopedPtr<IDeviceType>()>>;

class XAMP_OUTPUT_DEVICE_API XAMP_NO_VTABLE IAudioDeviceManager {
public:
	XAMP_BASE_CLASS(IAudioDeviceManager)

	virtual void initial() = 0;

	virtual void registerDeviceListener(const std::weak_ptr<IDeviceStateListener> & callback) = 0;

	virtual void registerDevice(const Uuid& id, std::function<ScopedPtr<IDeviceType>()> func) = 0;

	[[nodiscard]] virtual ScopedPtr<IDeviceType> createDefaultDeviceType() const = 0;

	[[nodiscard]] virtual ScopedPtr<IDeviceType> create(const Uuid& id) const = 0;

	[[nodiscard]] virtual std::vector<Uuid> getAvailableDeviceType() const = 0;

	virtual void clear() = 0;

	virtual DeviceTypeFactoryMap::iterator begin() = 0;

	virtual DeviceTypeFactoryMap::iterator end() = 0;

	virtual void shutdown() = 0;

	[[nodiscard]] virtual bool isSharedDevice(const Uuid& type) const = 0;

	[[nodiscard]] virtual bool isASIODevice(const Uuid& type) const = 0;
protected:	
	IAudioDeviceManager() = default;
};

XAMP_OUTPUT_DEVICE_NAMESPACE_END
