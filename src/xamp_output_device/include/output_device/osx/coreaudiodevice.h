//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <CoreAudio/CoreAudio.h>
#include <CoreServices/CoreServices.h>
#include <AudioUnit/AudioUnit.h>

#include <base/audioformat.h>
#include <base/align_ptr.h>
#include <base/dataconverter.h>
#include <output_device/device.h>

namespace xamp::output_device::osx {

using namespace base;

class XAMP_OUTPUT_DEVICE_API CoreAudioDevice final : public Device {
public:
    explicit CoreAudioDevice(AudioDeviceID device_id);

    ~CoreAudioDevice() override;

    void OpenStream(const AudioFormat &output_format) override;

    void SetAudioCallback(AudioCallback *callback) override;

    bool IsStreamOpen() const override;

    bool IsStreamRunning() const override;

    void StopStream() override;

    void CloseStream() override;

    void StartStream() override;

    void SetStreamTime(double stream_time) override;

    double GetStreamTime() const override;

    int32_t GetVolume() const override;

    void SetVolume(int32_t volume) const override;

    void SetMute(bool mute) const override;

    bool IsMuted() const override;

    void DisplayControlPanel() override;

    InterleavedFormat GetInterleavedFormat() const override;

    int32_t GetBufferSize() const override;
private:
    static OSStatus OnAudioIOProc(AudioDeviceID,
                                  const AudioTimeStamp*,
                                  const AudioBufferList*,
                                  const AudioTimeStamp*,
                                  AudioBufferList* outOutputData,
                                  const AudioTimeStamp*,
                                  void* user_data);

    void AudioIOProc(AudioBufferList* output_data);

    bool is_running_;
    AudioDeviceID device_id_;
    AudioDeviceIOProcID proc_id_;
    int32_t buffer_size_;
    AudioCallback *callback_;
    double stream_time_;
    mutable AudioObjectPropertyAddress audio_property_;
    AudioFormat format_;
};

}

