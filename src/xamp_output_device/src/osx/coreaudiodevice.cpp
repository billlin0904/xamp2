#include <vector>
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <mach/thread_policy.h>

#include <AudioToolbox/AudioToolbox.h>

#include <base/logger.h>

#include <output_device/osx/osx_utitl.h>
#include <output_device/audiocallback.h>
#include <output_device/osx/coreaudioexception.h>
#include <output_device/osx/coreaudiodevice.h>

namespace xamp::output_device::osx {

struct SystemVolume {
    explicit SystemVolume(AudioObjectPropertySelector selector, AudioDeviceID device_id = kAudioObjectUnknown) noexcept
        : device_id_ (device_id) {
        if (device_id != kAudioObjectUnknown) {
            property_.mElement  = kAudioObjectPropertyElementMaster;
            property_.mSelector = selector;
            property_.mScope    = kAudioDevicePropertyScopeOutput;
            return;
        }
        property_.mScope    = kAudioObjectPropertyScopeGlobal;
        property_.mElement  = kAudioObjectPropertyElementMaster;
        property_.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
        if (::AudioObjectHasProperty(kAudioObjectSystemObject, &property_)) {
            UInt32 deviceIDSize = sizeof (device_id_);
            OSStatus status = ::AudioObjectGetPropertyData(kAudioObjectSystemObject,
                                                           &property_,
                                                           0,
                                                           nullptr,
                                                           &deviceIDSize,
                                                           &device_id_);
            if (status == noErr) {
                property_.mElement  = kAudioObjectPropertyElementMaster;
                property_.mSelector = selector;
                property_.mScope    = kAudioDevicePropertyScopeOutput;
                if (!::AudioObjectHasProperty(device_id_, &property_)) {
                    device_id_ = kAudioObjectUnknown;
                }
            }
        }
    }

    double GetGain() const {
        Float32 gain = 0;
        if (device_id_ != kAudioObjectUnknown) {
            UInt32 size = sizeof(gain);
            CoreAudioThrowIfError(::AudioObjectGetPropertyData(device_id_,
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
            CoreAudioThrowIfError(::AudioObjectSetPropertyData(device_id_,
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
            CoreAudioThrowIfError(::AudioObjectGetPropertyData(device_id_,
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
            CoreAudioThrowIfError(::AudioObjectSetPropertyData(device_id_,
                                                               &property_,
                                                               0,
                                                               nullptr,
                                                               size,
                                                               &newMute));
        }
    }

    bool HasProperty() const {
        return ::AudioObjectHasProperty(device_id_, &property_) > 0;
    }

private:
    bool CanSetVolume() const noexcept {
        Boolean is_settable = false;
        return ::AudioObjectIsPropertySettable(device_id_,
                                               &property_,
                                               &is_settable) == noErr && is_settable;
    }

    AudioDeviceID device_id_;
    AudioObjectPropertyAddress property_;
};

CoreAudioDevice::CoreAudioDevice(AudioDeviceID device_id)
    : is_running_(false)
    , device_id_(device_id)
    , ioproc_id_(nullptr)
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

void CoreAudioDevice::OpenStream(AudioFormat const &output_format) {
    AudioStreamBasicDescription fmt;
    uint32 dataSize = sizeof(fmt);
    audio_property_.mSelector = kAudioStreamPropertyVirtualFormat;

    CoreAudioThrowIfError(::AudioObjectGetPropertyData(device_id_,
                                                       &audio_property_,
                                                       0,
                                                       nullptr,
                                                       &dataSize,
                                                       &fmt));

    if (fmt.mFormatFlags & kAudioFormatFlagIsNonInterleaved) {
        XAMP_LOG_DEBUG("Format is non interleaved.");
    } else {
        XAMP_LOG_DEBUG("Format is interleaved.");
    }

    if (!IsSupportSampleRate(device_id_, output_format.GetSampleRate())) {
        throw DeviceUnSupportedFormatException(output_format);
    }

    if (format_ != output_format) {
        fmt.mFormatID = kAudioFormatLinearPCM;
        fmt.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
        fmt.mSampleRate = output_format.GetSampleRate();
        fmt.mChannelsPerFrame = output_format.GetChannels();
        fmt.mFramesPerPacket = 1;
        fmt.mBitsPerChannel = output_format.GetBitsPerSample();
        fmt.mBytesPerFrame = output_format.GetBytesPerSample();
        fmt.mBytesPerPacket = output_format.GetBytesPerSample();
        fmt.mReserved = 0;
        auto error = ::AudioObjectSetPropertyData(device_id_,
                                                  &audio_property_,
                                                  0,
                                                  nullptr,
                                                  dataSize,
                                                  &fmt);
        if (error == kAudioCodecUnsupportedFormatError) {
            throw DeviceUnSupportedFormatException(output_format);
        }
        CoreAudioThrowIfError(error);
        XAMP_LOG_DEBUG("Update audio format {}.", output_format);
    }

    UInt32 bufferSize = 0;
    audio_property_.mSelector = kAudioDevicePropertyBufferFrameSize;
    dataSize = sizeof(bufferSize);
    CoreAudioThrowIfError(::AudioObjectGetPropertyData(device_id_,
                                                       &audio_property_,
                                                       0,
                                                       nullptr,
                                                       &dataSize,
                                                       &bufferSize));
    XAMP_LOG_DEBUG("Allocate buffer size:{}.", bufferSize);

    UInt32 theSize = bufferSize;
    dataSize = sizeof(UInt32);
    audio_property_.mSelector = kAudioDevicePropertyBufferFrameSize;
    CoreAudioThrowIfError(::AudioObjectSetPropertyData(device_id_,
                                                       &audio_property_,
                                                       0,
                                                       nullptr,
                                                       dataSize,
                                                       &theSize));

    buffer_size_ = output_format.GetChannels() * bufferSize;

    CoreAudioThrowIfError(::AudioDeviceCreateIOProcID(device_id_,
                                                      OnAudioDeviceIOProc,
                                                      this,
                                                      &ioproc_id_));
    format_ = output_format;

    ReleaseHogMode(device_id_);
    SetHogMode(device_id_);
}

void CoreAudioDevice::SetAudioCallback(AudioCallback *callback) noexcept {
    callback_ = callback;
}

bool CoreAudioDevice::IsStreamOpen() const noexcept {
    return ioproc_id_ != nullptr;
}

bool CoreAudioDevice::IsStreamRunning() const noexcept {
    return is_running_;
}

void CoreAudioDevice::StopStream(bool /*wait_for_stop_stream*/) {
    CoreAudioThrowIfError(::AudioDeviceStop(device_id_, ioproc_id_));
    is_running_ = false;
}

void CoreAudioDevice::CloseStream() {
    CoreAudioThrowIfError(::AudioDeviceStop(device_id_, ioproc_id_));
    CoreAudioThrowIfError(::AudioDeviceDestroyIOProcID(device_id_, ioproc_id_));
    ioproc_id_ = nullptr;
    ReleaseHogMode(device_id_);
}

void CoreAudioDevice::StartStream() {
    CoreAudioThrowIfError(::AudioDeviceStart(device_id_, ioproc_id_));
    is_running_ = true;
}

void CoreAudioDevice::SetStreamTime(double stream_time) noexcept {
    stream_time_ = stream_time
                   * static_cast<double>(format_.GetAvgFramesPerSec());
}

double CoreAudioDevice::GetStreamTime() const noexcept {
    return stream_time_;
}

uint32_t CoreAudioDevice::GetVolume() const {
    auto volume = SystemVolume(kAudioHardwareServiceDeviceProperty_VirtualMasterVolume, device_id_)
                      .GetGain() * 100;
    return static_cast<uint32_t>(volume);
}

void CoreAudioDevice::SetVolume(uint32_t volume) const {
    SystemVolume(kAudioHardwareServiceDeviceProperty_VirtualMasterVolume, device_id_)
        .SetGain(float(volume) / float(100.0));
}

void CoreAudioDevice::SetMute(bool mute) const {
    SystemVolume(kAudioDevicePropertyMute, device_id_).SetMuted(mute);
}

bool CoreAudioDevice::IsMuted() const {
    return SystemVolume(kAudioDevicePropertyMute, device_id_).IsMuted();
}

bool CoreAudioDevice::CanHardwareControlVolume() const {
    return SystemVolume(kAudioDevicePropertyMute, device_id_).HasProperty();
}

void CoreAudioDevice::DisplayControlPanel() {
}

InterleavedFormat CoreAudioDevice::GetInterleavedFormat() const noexcept {
    return InterleavedFormat::INTERLEAVED;
}

uint32_t CoreAudioDevice::GetBufferSize() const noexcept {
    return buffer_size_;
}

OSStatus CoreAudioDevice::OnAudioDeviceIOProc(AudioDeviceID,
                                              AudioTimeStamp const*,
                                              AudioBufferList const*,
                                              AudioTimeStamp const*,
                                              AudioBufferList *output_data,
                                              AudioTimeStamp const*,
                                              void *user_data) {
    auto device = static_cast<CoreAudioDevice*>(user_data);
    device->AudioDeviceIOProc(output_data);
    return noErr;
}

void CoreAudioDevice::AudioDeviceIOProc(AudioBufferList *output_data) {
    auto const buffer_count = output_data->mNumberBuffers;

    for (uint32_t i = 0; i < buffer_count; ++i) {
        const auto buffer = output_data->mBuffers[i];
        const uint32_t num_sample = static_cast<uint32_t>(buffer.mDataByteSize
                                                          / sizeof(float)
                                                          / format_.GetChannels());
        stream_time_ = stream_time_ + num_sample * 2;
        if (XAMP_UNLIKELY(callback_->OnGetSamples(static_cast<float*>(buffer.mData),
                                                  num_sample,
                                                  stream_time_ / static_cast<double>(format_.GetAvgFramesPerSec())) == 0)) {
            is_running_ = false;
            break;
        }
    }
}

}
