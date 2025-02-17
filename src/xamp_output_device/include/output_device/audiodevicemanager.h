//=====================================================================================================================
// Copyright (c) 2018-2025 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/stl.h>
#include <base/uuid.h>
#include <base/memory.h>
#include <base/pimplptr.h>

#include <output_device/iaudiodevicemanager.h>
#include <output_device/idevicetype.h>

XAMP_OUTPUT_DEVICE_NAMESPACE_BEGIN

/*
* AudioDeviceManager is the audio device manager.
*/
class AudioDeviceManager final : public IAudioDeviceManager {
public:
    XAMP_DISABLE_COPY(AudioDeviceManager)

    /*
    * Constructor
    */
    AudioDeviceManager();

    /*
    * Destructor
    */
    virtual ~AudioDeviceManager() override;    

    /*
	* Register device listener
	* 
	* @param callback: device state listener
	*/
    void RegisterDeviceListener(const std::weak_ptr<IDeviceStateListener> & callback) override;

    /*
    * Register device type
    *
    * @param id: device type id
    */
    void RegisterDevice(Uuid const& id, std::function<ScopedPtr<IDeviceType>()> func) override;

    /*
    * Clear all device type
    *
    */
    void Clear() override;

    /*
    * Create default device type
    *
    * @return default device type
    */
    XAMP_NO_DISCARD ScopedPtr<IDeviceType> CreateDefaultDeviceType() const override;

    /*
    * Create device type
    *
    * @param id: device type id
    */
    XAMP_NO_DISCARD ScopedPtr<IDeviceType> Create(Uuid const& id) const override;

    /*
    * Begin iterator
    *
    * @return begin iterator
    */
    DeviceTypeFactoryMap::iterator Begin() override;

    /*
    * End iterator
    *
    * @return end iterator
    */
    DeviceTypeFactoryMap::iterator End() override;

    /*
    * Get available device type
    *
    * @return available device type
    */
    XAMP_NO_DISCARD std::vector<Uuid> GetAvailableDeviceType() const override;

    /*
    * Is support asio
    * 
    * @return true if support asio
    */
    XAMP_NO_DISCARD bool IsSupportAsio() const noexcept;

    /*
    * Is device type exist
    * 
    * @param id: device type id
    * @return true if device type exist
    */
    XAMP_NO_DISCARD bool IsDeviceTypeExist(const Uuid& id) const noexcept;

    /*
    * Is shared device
    * 
    * @param type: device type
    */
    XAMP_NO_DISCARD bool IsSharedDevice(const Uuid& type) const noexcept override;

    /*
    * Shutdown global device resource.
    */
    void Shutdown() override;
private:
    class DeviceStateNotificationImpl;
    ScopedPtr<DeviceStateNotificationImpl> impl_;    
    DeviceTypeFactoryMap factory_;
};

XAMP_OUTPUT_DEVICE_NAMESPACE_END

