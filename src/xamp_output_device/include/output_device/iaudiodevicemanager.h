//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
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

/*
* IAudioDeviceManager is the audio device manager interface.
* 
*/
class XAMP_OUTPUT_DEVICE_API XAMP_NO_VTABLE IAudioDeviceManager {
public:
	XAMP_BASE_CLASS(IAudioDeviceManager)

	/*
	* Register device listener.
	* 
	* @param callback: device state listener.
	*/
	virtual void RegisterDeviceListener(const std::weak_ptr<IDeviceStateListener> & callback) = 0;

	/*
	* Register device type.
	* 
	* @param id: device type id.
	*/
	virtual void RegisterDevice(const Uuid& id, std::function<ScopedPtr<IDeviceType>()> func) = 0;

	/*
	* Create default device type.
	* 
	* @return default device type.
	*/
	[[nodiscard]] virtual ScopedPtr<IDeviceType> CreateDefaultDeviceType() const = 0;

	/*
	* Create device type.
	* 
	* @param id: device type id.
	*/
	[[nodiscard]] virtual ScopedPtr<IDeviceType> Create(const Uuid& id) const = 0;

	/*
	* Get available device type.
	* 
	* @return available device type.
	*/
	[[nodiscard]] virtual std::vector<Uuid> GetAvailableDeviceType() const = 0;

	/*
	* Clear all device type.
	* 
	*/
	virtual void Clear() = 0;

	/*
	* Begin iterator.
	* 
	* @return begin iterator.
	*/	
	virtual DeviceTypeFactoryMap::iterator Begin() = 0;

	/*
	* End iterator
	* 
	* @return end iterator
	*/	
	virtual DeviceTypeFactoryMap::iterator End() = 0;

	/*
	* Shutdown global device resource.
	*/
	virtual void Shutdown() = 0;

	/*
	* Is shared device
	*
	* @param type: device type
	*/
	[[nodiscard]] virtual bool IsSharedDevice(const Uuid& type) const noexcept = 0;
protected:
	/*
	* Constructor.
	*/	
	IAudioDeviceManager() = default;
};

XAMP_OUTPUT_DEVICE_NAMESPACE_END