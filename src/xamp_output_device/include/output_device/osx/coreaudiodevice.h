//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

#ifdef XAMP_OS_MAC

#include <CoreAudio/CoreAudio.h>
#include <CoreServices/CoreServices.h>
#include <AudioUnit/AudioUnit.h>

#include <base/logger.h>
#include <base/fastmutex.h>
#include <base/fastconditionvariable.h>
#include <base/audioformat.h>
#include <base/memory.h>
#include <base/dataconverter.h>
#include <output_device/ioutputdevice.h>

namespace xamp::output_device::osx {

class XAMP_OUTPUT_DEVICE_API CoreAudioDevice final : public IOutputDevice {
public:
    CoreAudioDevice(AudioDeviceID device_id, bool is_hog_mode);

    virtual ~CoreAudioDevice() override;

    void OpenStream(AudioFormat const &output_format) override;

    void SetAudioCallback(IAudioCallback *callback) noexcept override;

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

    PackedFormat GetPackedFormat() const noexcept override;

    uint32_t GetBufferSize() const noexcept override;

    bool IsHardwareControlVolume() const override;

    void AbortStream() noexcept override;

    void SetVolumeLevelScalar(float level) override;

    void SetBlance();
    
private:
    static OSStatus OnAudioDeviceIOProc(AudioDeviceID,
                                        AudioTimeStamp const*,
                                        AudioBufferList const *,
                                        AudioTimeStamp const*,
                                        AudioBufferList* outOutputData,
                                        AudioTimeStamp const*,
                                        void* user_data);

    void FillSamples(AudioBufferList* output_data, double device_sample_time, bool is_slient);

    uint32_t GetHardwareLantency(AudioDeviceID device_id, AudioObjectPropertyScope scope);

    bool is_running_;
    bool is_hog_mode_;
    AudioDeviceID device_id_;
    AudioDeviceIOProcID ioproc_id_;
    uint32_t buffer_size_;
    uint32_t latency_;
    IAudioCallback *callback_;
    std::atomic<double> stream_time_;
    mutable AudioObjectPropertyAddress audio_property_;
    AudioFormat format_;
    FastMutex mutex_;
    FastConditionVariable stop_event_;
    std::shared_ptr<Logger> logger_;
};

}

#endif

