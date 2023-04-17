//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/stl.h>
#include <base/uuid.h>
#include <base/align_ptr.h>
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
    void RegisterDeviceListener(std::weak_ptr<IDeviceStateListener> const & callback) override;

    /*
    * Register device type
    *
    * @param id: device type id
    */
    void RegisterDevice(Uuid const& id, std::function<AlignPtr<IDeviceType>()> func) override;

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
    [[nodiscard]] AlignPtr<IDeviceType> CreateDefaultDeviceType() const override;

    /*
    * Create device type
    *
    * @param id: device type id
    */
    [[nodiscard]] AlignPtr<IDeviceType> Create(Uuid const& id) const override;

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
    [[nodiscard]] Vector<Uuid> GetAvailableDeviceType() const override;

    /*
    * Is support asio
    * 
    * @return true if support asio
    */
    [[nodiscard]] bool IsSupportAsio() const noexcept;

    /*
    * Is device type exist
    * 
    * @param id: device type id
    * @return true if device type exist
    */
    [[nodiscard]] bool IsDeviceTypeExist(Uuid const& id) const noexcept;    
private:
    class DeviceStateNotificationImpl;
    PimplPtr<DeviceStateNotificationImpl> impl_;    
    DeviceTypeFactoryMap factory_;
};

XAMP_OUTPUT_DEVICE_NAMESPACE_END

