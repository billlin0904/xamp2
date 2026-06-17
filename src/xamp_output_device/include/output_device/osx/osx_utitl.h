//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#include <base/base.h>

#ifdef XAMP_OS_MAC

#include <vector>
#include <string>

#include <output_device/deviceinfo.h>
#include <output_device/osx/coreaudioexception.h>

#include <AudioToolbox/AudioToolbox.h>
#include <CoreAudio/CoreAudio.h>

namespace xamp::output_device::osx {

struct SystemVolume {
    explicit SystemVolume(AudioObjectPropertySelector selector, AudioDeviceID device_id = kAudioObjectUnknown) ;

    double getGain() const;

    void setGain(float gain) const;

    float getBlance(AudioObjectPropertyScope scope = kAudioDevicePropertyScopeOutput) const;

    void setBlance(float blance, AudioObjectPropertyScope scope = kAudioDevicePropertyScopeOutput);

    bool isMuted() const;

    void setMuted(bool mute) const;

    bool hasProperty() const ;

    bool hasProperty(const AudioObjectPropertyAddress &property) const ;

    bool canSetVolume() const ;
private:
    AudioDeviceID device_id_;
    AudioObjectPropertyAddress property_;
};

std::vector<uint32_t> GetAvailableSampleRates(AudioDeviceID id);

bool IsSupportSampleRate(AudioDeviceID id, uint32_t samplerate);

bool IsSupportDopMode(AudioDeviceID id);

std::string GetDeviceUid(AudioDeviceID id);

std::wstring GetDeviceName(AudioDeviceID id, AudioObjectPropertySelector selector);

std::wstring GetPropertyName(AudioDeviceID id);

AudioDeviceID GetAudioDeviceIdByUid(bool is_input, std::string const & device_id);

bool IsOutputDevice(AudioDeviceID id);

void SetHogMode(AudioDeviceID id);

bool SetAutoHogMode(bool enable);

bool IsAutoHogMode();

void ReleaseHogMode(AudioDeviceID id);

bool CanSetHogMode(AudioDeviceID id);

std::vector<std::string> GetSystemUsbPath();

DeviceConnectType GetDeviceConnectType(AudioDeviceID id);

}

#endif
