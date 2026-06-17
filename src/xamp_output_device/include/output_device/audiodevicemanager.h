//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/stl.h>
#include <base/uuid.h>
#include <base/memory.h>
#include <base/memory.h>

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
    * initial audio device manager.
    */
    void initial() override;

    /*
	* Register device listener
	* 
	* @param callback: device state listener
	*/
    void registerDeviceListener(const std::weak_ptr<IDeviceStateListener> & callback) override;

    /*
    * Register device type
    *
    * @param id: device type id
    */
    void registerDevice(Uuid const& id, std::function<ScopedPtr<IDeviceType>()> func) override;

    /*
    * Clear all device type
    *
    */
    void clear() override;

    /*
    * create default device type
    *
    * @return default device type
    */
    [[nodiscard]] ScopedPtr<IDeviceType> createDefaultDeviceType() const override;

    /*
    * create device type
    *
    * @param id: device type id
    */
    [[nodiscard]] ScopedPtr<IDeviceType> create(Uuid const& id) const override;

    /*
    * Begin iterator
    *
    * @return begin iterator
    */
    DeviceTypeFactoryMap::iterator begin() override;

    /*
    * End iterator
    *
    * @return end iterator
    */
    DeviceTypeFactoryMap::iterator end() override;

    /*
    * Get available device type
    *
    * @return available device type
    */
    [[nodiscard]] std::vector<Uuid> getAvailableDeviceType() const override;

    /*
    * Is support asio
    * 
    * @return true if support asio
    */
    [[nodiscard]] bool isSupportAsio() const ;

    /*
    * Is device type exist
    * 
    * @param id: device type id
    * @return true if device type exist
    */
    [[nodiscard]] bool isDeviceTypeExist(const Uuid& id) const ;

    /*
    * Is shared device
    * 
    * @param type: device type
    */
    [[nodiscard]] bool isSharedDevice(const Uuid& type) const override;

    /*
    * Is ASIO device
    *
    * @param type: device type
    */
    [[nodiscard]] bool isASIODevice(const Uuid& type) const override;

    /*
    * shutdown global device resource.
    */
    void shutdown() override;
private:    
    bool is_initialized_{ false };
    class DeviceStateNotificationImpl;
    ScopedPtr<DeviceStateNotificationImpl> impl_;        
    DeviceTypeFactoryMap factory_;    
};

XAMP_OUTPUT_DEVICE_NAMESPACE_END
