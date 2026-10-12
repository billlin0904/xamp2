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
#include <output_device/idsddevice.h>
#include <vector>

namespace xamp::output_device::osx {

class XAMP_OUTPUT_DEVICE_API CoreAudioDevice final : public IOutputDevice, public IDsdDevice {
public:
    CoreAudioDevice(AudioDeviceID device_id, bool is_hog_mode);

    virtual ~CoreAudioDevice() override;

    void setBitPerfect(bool enabled) override;
    void setIntegerPcmFormat(const xamp::pcm::Format& format) override;
    void setIoFormat(DsdIoFormat format) override;
    DsdIoFormat getIoFormat() const override;

    void openStream(AudioFormat const &output_format) override;

    void setAudioCallback(IAudioCallback *callback) override;

    bool isStreamOpen() const override;

    bool isStreamRunning() const override;

    void stopStream(bool wait_for_stop_stream = true) override;

    void closeStream() override;

    void startStream() override;

    void setStreamTime(double stream_time) override;

    double getStreamTime() const override;

    uint32_t getVolume() const override;

    void setVolume(uint32_t volume) const override;

    void setMute(bool mute) const override;

    bool isMuted() const override;

    PackedFormat getPackedFormat() const override;

    uint32_t getBufferSize() const override;

    bool isHardwareControlVolume() const override;

    void abortStream() override;

    void setVolumeLevelScalar(float level);

    void setBlance();
    
private:
    static OSStatus OnAudioDeviceIOProc(AudioDeviceID,
                                        AudioTimeStamp const*,
                                        AudioBufferList const *,
                                        AudioTimeStamp const*,
                                        AudioBufferList* outOutputData,
                                        AudioTimeStamp const*,
                                        void* user_data);

    void fillSamples(AudioBufferList* output_data, double device_sample_time, bool is_slient);

    uint32_t getHardwareLantency(AudioDeviceID device_id, AudioObjectPropertyScope scope);

    static OSStatus OnIntegerPropertyChanged(AudioObjectID, UInt32,
        const AudioObjectPropertyAddress*, void* user_data);
    void removeIntegerListeners() noexcept;
    std::atomic<bool> integer_format_changed_{false};
    bool integer_error_reported_{false};
    unsigned integer_listener_count_{0};

    void openIntegerStream(const AudioFormat& format);
    void restoreIntegerStream() noexcept;
    void fillIntegerSamples(AudioBufferList* output, double sample_time);

    std::atomic<bool> is_running_;
    bool bitperfect_{false};
    DsdIoFormat io_format_{DsdIoFormat::IO_FORMAT_PCM};
    xamp::pcm::Format source_pcm_{};
    xamp::pcm::Format device_pcm_{};
    std::vector<std::byte> pcm_buffer_;
    AudioStreamID integer_stream_{kAudioObjectUnknown};
    AudioStreamBasicDescription negotiated_format_{};
    AudioStreamBasicDescription original_physical_{};
    AudioStreamBasicDescription original_virtual_{};
    Float64 original_rate_{0};
    UInt32 original_mixing_{0};
    bool restore_formats_{false};
    bool restore_rate_{false};
    bool restore_mixing_{false};
    bool owns_hog_{false};
    uint8_t dop_marker_{0x05};
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

