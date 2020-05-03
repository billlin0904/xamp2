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

    void SetAudioCallback(AudioCallback *callback) noexcept override;

    bool IsStreamOpen() const noexcept override;

    bool IsStreamRunning() const noexcept override;

    void StopStream(bool wait_for_stop_stream = true) override;

    void CloseStream() override;

    void StartStream() override;

    void SetStreamTime(double stream_time) noexcept override;

    double GetStreamTime() const noexcept override;

    uint32_t GetVolume() const override;

    void SetVolume(uint32_t volume) const override;

    void SetMute(bool mute) const override;

    bool IsMuted() const override;

    void DisplayControlPanel() override;

    InterleavedFormat GetInterleavedFormat() const noexcept override;

    uint32_t GetBufferSize() const noexcept override;

    bool CanHardwareControlVolume() const override;
private:
    static OSStatus OnAudioDeviceIOProc(AudioDeviceID,
                                        const AudioTimeStamp*,
                                        const AudioBufferList*,
                                        const AudioTimeStamp*,
                                        AudioBufferList* outOutputData,
                                        const AudioTimeStamp*,
                                        void* user_data);

    void AudioDeviceIOProc(AudioBufferList* output_data);

    bool is_running_;
    AudioDeviceID device_id_;
    AudioDeviceIOProcID proc_id_;
    uint32_t buffer_size_;
    AudioCallback *callback_;
    double stream_time_;
    mutable AudioObjectPropertyAddress audio_property_;
    AudioFormat format_;
};

}

