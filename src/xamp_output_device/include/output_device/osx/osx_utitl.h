//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#include <vector>
#include <string>

#include <CoreAudio/CoreAudio.h>

namespace xamp::output_device::osx {

std::vector<uint32_t> GetAvailableSampleRates(AudioDeviceID id);

bool IsSupportSampleRate(AudioDeviceID id, uint32_t samplerate);

bool IsSupportDopMode(AudioDeviceID id);

std::wstring GetDeviceUid(AudioDeviceID id);

std::wstring GetDeviceName(AudioDeviceID id, AudioObjectPropertySelector selector);

std::wstring GetPropertyName(AudioDeviceID id);

AudioDeviceID GetAudioDeviceIdByUid(bool is_input, const std::wstring& device_id);

bool IsOutputDevice(AudioDeviceID id);

}

