//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <output_device/output_device.h>
#include <output_device/idevicetype.h>
#include <output_device/idevicestatelistener.h>

#include <base/stl.h>

#include <map>

XAMP_OUTPUT_DEVICE_NAMESPACE_BEGIN

using DeviceTypeFactoryMap = OrderedMap<Uuid, std::function<AlignPtr<IDeviceType>()>>;

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
	virtual void RegisterDevice(const Uuid& id, std::function<AlignPtr<IDeviceType>()> func) = 0;

	/*
	* Create default device type.
	* 
	* @return default device type.
	*/
	[[nodiscard]] virtual AlignPtr<IDeviceType> CreateDefaultDeviceType() const = 0;

	/*
	* Create device type.
	* 
	* @param id: device type id.
	*/
	[[nodiscard]] virtual AlignPtr<IDeviceType> Create(const Uuid& id) const = 0;

	/*
	* Get available device type.
	* 
	* @return available device type.
	*/
	[[nodiscard]] virtual Vector<Uuid> GetAvailableDeviceType() const = 0;

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

	virtual void Shutdown() = 0;
protected:
	/*
	* Constructor.
	*/	
	IAudioDeviceManager() = default;
};

XAMP_OUTPUT_DEVICE_NAMESPACE_END