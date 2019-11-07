#include <vector>

#include <AudioToolbox/AudioToolbox.h>

#include <base/logger.h>

#include <output_device/audiocallback.h>
#include <output_device/osx/coreaudioexception.h>
#include <output_device/osx/coreaudiodevice.h>

namespace xamp::output_device::osx {

struct SystemVolume {
    explicit SystemVolume(AudioObjectPropertySelector selector) noexcept
        : device_id_ (kAudioObjectUnknown) {
        property_.mScope    = kAudioObjectPropertyScopeGlobal;
        property_.mElement  = kAudioObjectPropertyElementMaster;
        property_.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
        if (AudioObjectHasProperty(kAudioObjectSystemObject, &property_)) {
            UInt32 deviceIDSize = sizeof (device_id_);
            OSStatus status = AudioObjectGetPropertyData (kAudioObjectSystemObject, &property_, 0, nullptr, &deviceIDSize, &device_id_);
            if (status == noErr) {
                property_.mElement  = kAudioObjectPropertyElementMaster;
                property_.mSelector = selector;
                property_.mScope    = kAudioDevicePropertyScopeOutput;
                if (! AudioObjectHasProperty(device_id_, &property_))
                    device_id_ = kAudioObjectUnknown;
            }
        }
    }

    double GetGain() const {
        Float32 gain = 0;
        if (device_id_ != kAudioObjectUnknown) {
            UInt32 size = sizeof(gain);
            CoreAudioThrowIfError(AudioObjectGetPropertyData(device_id_,
                                                             &property_,
                                                             0,
                                                             nullptr,
                                                             &size,
                                                             &gain));
        }
        return static_cast<double>(gain);
    }

    void SetGain(float gain) const {
        if (device_id_ != kAudioObjectUnknown && CanSetVolume()) {
            Float32 newVolume = gain;
            UInt32 size = sizeof(newVolume);
            CoreAudioThrowIfError(AudioObjectSetPropertyData(device_id_,
                                                             &property_,
                                                             0,
                                                             nullptr,
                                                             size,
                                                             &newVolume));
        }
    }

    bool IsMuted() const {
        UInt32 muted = 0;
        if (device_id_ != kAudioObjectUnknown) {
            UInt32 size = sizeof(muted);
            CoreAudioThrowIfError(AudioObjectGetPropertyData(device_id_,
                                                             &property_,
                                                             0,
                                                             nullptr,
                                                             &size,
                                                             &muted));
        }
        return muted != 0;
    }

    void SetMuted(bool mute) const {
        if (device_id_ != kAudioObjectUnknown && CanSetVolume()) {
            UInt32 newMute = mute ? 1 : 0;
            UInt32 size = sizeof(newMute);
            CoreAudioThrowIfError(AudioObjectSetPropertyData(device_id_,
                                                             &property_,
                                                             0,
                                                             nullptr,
                                                             size,
                                                             &newMute));
        }
    }

private:
    bool CanSetVolume() const noexcept {
        Boolean is_settable = false;
        return AudioObjectIsPropertySettable(device_id_,
                                             &property_,
                                             &is_settable) == noErr && is_settable;
    }

    AudioDeviceID device_id_;
    AudioObjectPropertyAddress property_;
};

CoreAudioDevice::CoreAudioDevice(AudioDeviceID device_id)
    : is_running_(false)
    , device_id_(device_id)
    , proc_id_(nullptr)
    , buffer_size_(0)
    , callback_(nullptr)
    , stream_time_(0) {
    audio_property_.mScope = kAudioDevicePropertyScopeOutput;
    audio_property_.mElement = kAudioObjectPropertyElementMaster;
}

CoreAudioDevice::~CoreAudioDevice() {
    try {
        StopStream();
        CloseStream();
    } catch (...) {
    }
}

void CoreAudioDevice::OpenStream(const AudioFormat &output_format) {
    AudioStreamBasicDescription fmt;
    uint32 dataSize = sizeof(fmt);
    audio_property_.mSelector = kAudioStreamPropertyVirtualFormat;

    CoreAudioThrowIfError(AudioObjectGetPropertyData(device_id_,
                                                     &audio_property_,
                                                     0,
                                                     nullptr,
                                                     &dataSize,
                                                     &fmt));

    if (fmt.mFormatFlags & kAudioFormatFlagIsNonInterleaved) {
        XAMP_LOG_DEBUG("Format is non interleaved");
    } else {
        XAMP_LOG_DEBUG("Format is interleaved");
    }

    if (format_ != output_format) {
        fmt.mFormatID = kAudioFormatLinearPCM;
        fmt.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
        fmt.mSampleRate = output_format.GetSampleRate();
        fmt.mChannelsPerFrame = (UInt32)output_format.GetChannels();
        fmt.mFramesPerPacket = 1;
        fmt.mBitsPerChannel = (UInt32)output_format.GetBitsPerSample();
        fmt.mBytesPerFrame = (UInt32)output_format.GetBytesPerSample();
        fmt.mBytesPerPacket = (UInt32)output_format.GetBytesPerSample();
        fmt.mReserved = 0;
        CoreAudioThrowIfError(AudioObjectSetPropertyData(device_id_,
                                                         &audio_property_,
                                                         0,
                                                         nullptr,
                                                         dataSize,
                                                         &fmt));
        XAMP_LOG_DEBUG("Update audio format {}", output_format);
    }

    UInt32 bufferSize = 0;
    audio_property_.mSelector = kAudioDevicePropertyBufferFrameSize;
    dataSize = sizeof(bufferSize);
    CoreAudioThrowIfError(AudioObjectGetPropertyData(
                              device_id_,
                              &audio_property_,
                              0,
                              nullptr,
                              &dataSize,
                              &bufferSize));
    XAMP_LOG_DEBUG("Allocate buffer size:{}", bufferSize);

    UInt32 theSize = bufferSize;
    dataSize = sizeof(UInt32);
    audio_property_.mSelector = kAudioDevicePropertyBufferFrameSize;
    CoreAudioThrowIfError(AudioObjectSetPropertyData(
                              device_id_,
                              &audio_property_,
                              0,
                              nullptr,
                              dataSize,
                              &theSize));

    buffer_size_ = (uint32_t)output_format.GetChannels() * bufferSize;

    CoreAudioThrowIfError(AudioDeviceCreateIOProcID(device_id_, OnAudioIOProc, this, &proc_id_));
    format_ = output_format;
}

void CoreAudioDevice::SetAudioCallback(AudioCallback *callback) {
    callback_ = callback;
}

bool CoreAudioDevice::IsStreamOpen() const {
    return proc_id_ != nullptr;
}

bool CoreAudioDevice::IsStreamRunning() const {
    return is_running_;
}

void CoreAudioDevice::StopStream(bool wait_for_stop_stream) {
    CoreAudioThrowIfError(AudioDeviceStop(device_id_, proc_id_));
    is_running_ = false;
}

void CoreAudioDevice::CloseStream() {
    CoreAudioThrowIfError(AudioDeviceStop(device_id_, proc_id_));
    CoreAudioThrowIfError(AudioDeviceDestroyIOProcID(device_id_, proc_id_));
    proc_id_ = nullptr;
}

void CoreAudioDevice::StartStream() {
    CoreAudioThrowIfError(AudioDeviceStart(device_id_, proc_id_));
    is_running_ = true;
}

void CoreAudioDevice::SetStreamTime(double stream_time) {
    stream_time_ = stream_time * format_.GetAvgFramesPerSec();
}

double CoreAudioDevice::GetStreamTime() const {
    return stream_time_;
}

int32_t CoreAudioDevice::GetVolume() const {
    auto volume = SystemVolume(kAudioHardwareServiceDeviceProperty_VirtualMasterVolume).GetGain() * 100;
    return (int32_t)volume;
}

void CoreAudioDevice::SetVolume(int32_t volume) const {
    SystemVolume(kAudioHardwareServiceDeviceProperty_VirtualMasterVolume).SetGain(volume / 100.0);
}

void CoreAudioDevice::SetMute(bool mute) const {
    SystemVolume(kAudioDevicePropertyMute).SetMuted(mute);
}

bool CoreAudioDevice::IsMuted() const {
    return SystemVolume(kAudioDevicePropertyMute).IsMuted();
}

void CoreAudioDevice::DisplayControlPanel() {
}

InterleavedFormat CoreAudioDevice::GetInterleavedFormat() const {
    return InterleavedFormat::INTERLEAVED;
}

int32_t CoreAudioDevice::GetBufferSize() const {
    return buffer_size_;
}

OSStatus CoreAudioDevice::OnAudioIOProc(AudioDeviceID,
                                        const AudioTimeStamp *,
                                        const AudioBufferList *,
                                        const AudioTimeStamp *,
                                        AudioBufferList *output_data,
                                        const AudioTimeStamp *,
                                        void *user_data) {
    auto device = static_cast<CoreAudioDevice*>(user_data);
    device->AudioIOProc(output_data);
    return noErr;
}

void CoreAudioDevice::AudioIOProc(AudioBufferList *output_data) {
    const auto buffer_count = output_data->mNumberBuffers;

    for (uint32_t i = 0; i < buffer_count; ++i) {
        const auto buffer = output_data->mBuffers[i];
        const auto numSample = static_cast<int32_t>(buffer.mDataByteSize / sizeof(float) / format_.GetChannels());
        stream_time_ = stream_time_ + static_cast<double>(numSample * 2);
        if (XAMP_UNLIKELY( (*callback_)(static_cast<float*>(buffer.mData), numSample, stream_time_ / format_.GetAvgFramesPerSec()) == 0 )) {
            is_running_ = false;
            break;
        }
    }
}

}
