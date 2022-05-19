#include <vector>
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <mach/thread_policy.h>
#include <time.h>

#include <AudioToolbox/AudioToolbox.h>

#include <base/stl.h>
#include <base/logger.h>
#include <base/singleton.h>
#include <base/memory.h>
#include <base/timer.h>
#include <base/platform.h>

#include <output_device/osx/osx_utitl.h>
#include <output_device/iaudiocallback.h>
#include <output_device/osx/coreaudioexception.h>
#include <output_device/osx/coreaudiodevice.h>

namespace xamp::output_device::osx {

CoreAudioDevice::CoreAudioDevice(AudioDeviceID device_id, bool is_hog_mode)
    : is_running_(false)
    , is_hog_mode_(is_hog_mode)
    , device_id_(device_id)
    , ioproc_id_(nullptr)
    , buffer_size_(0)
    , latency_(0)
    , callback_(nullptr)
    , stream_time_(0) {
    audio_property_.mScope = kAudioDevicePropertyScopeOutput;
    audio_property_.mElement = kAudioObjectPropertyElementMaster;
    log_ = Logger::GetInstance().GetLogger(kCoreAudioLoggerName);
}

CoreAudioDevice::~CoreAudioDevice() {
    if (is_hog_mode_) {
        ReleaseHogMode(device_id_);
    }

    try {
        StopStream();
        CloseStream();
    } catch (...) {
    }
}

void CoreAudioDevice::SetBlance() {
    auto main_volume = SystemVolume(kAudioHardwareServiceDeviceProperty_VirtualMainVolume, device_id_);
    try {
        const auto blance = main_volume.GetBlance();
        if (blance != 0.5f) {
            XAMP_LOG_D(log_, "Device volume not blance: {}!", blance);
            main_volume.SetBlance(0.5);
        }
    } catch (Exception const &e) {
        XAMP_LOG_D(log_, "Failure to set volume blance, {}!", e.GetErrorMessage());
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
        XAMP_LOG_D(log_, "Format is non interleaved.");
    } else {
        XAMP_LOG_D(log_, "Format is interleaved.");
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
        XAMP_LOG_D(log_, "Update audio format {}.", output_format);
    }

    UInt32 buffer_size = 0;
    audio_property_.mSelector = kAudioDevicePropertyBufferFrameSize;
    dataSize = sizeof(buffer_size);
    CoreAudioThrowIfError(::AudioObjectGetPropertyData(device_id_,
                                                       &audio_property_,
                                                       0,
                                                       nullptr,
                                                       &dataSize,
                                                       &buffer_size));
    XAMP_LOG_D(log_, "Allocate buffer size:{}.", buffer_size);

    UInt32 size = buffer_size;
    dataSize = sizeof(UInt32);
    audio_property_.mSelector = kAudioDevicePropertyBufferFrameSize;
    CoreAudioThrowIfError(::AudioObjectSetPropertyData(device_id_,
                                                       &audio_property_,
                                                       0,
                                                       nullptr,
                                                       dataSize,
                                                       &size));
    XAMP_LOG_D(log_, "Set buffer size:{}.", size);

    buffer_size_ = output_format.GetChannels() * buffer_size;

    latency_ = GetHardwareLantency(device_id_, kAudioDevicePropertyScopeOutput);

    CoreAudioThrowIfError(::AudioDeviceCreateIOProcID(device_id_,
                                                      OnAudioDeviceIOProc,
                                                      this,
                                                      &ioproc_id_));
    format_ = output_format;

    /*
    if (is_hog_mode_) {
        SetAutoHogMode(true);
    } else {
        SetAutoHogMode(false);
    }

    auto enable_auto_hog = IsAutoHogMode();
    if (!enable_auto_hog) {
        if (is_hog_mode_) {
            XAMP_LOG_D(logger_, "Set auto hog mode failure, fallback use set hog mode.");
            ReleaseHogMode(device_id_);
            SetHogMode(device_id_);
        }
    } else {
        XAMP_LOG_D(logger_, "Set auto hog mode!");
    }
    */

    if (is_hog_mode_) {
        XAMP_LOG_D(log_, "Set hog mode!");
        ReleaseHogMode(device_id_);
        SetHogMode(device_id_);
    }

    SetBlance();
}

void CoreAudioDevice::SetAudioCallback(IAudioCallback *callback) noexcept {
    callback_ = callback;
}

bool CoreAudioDevice::IsStreamOpen() const noexcept {
    return ioproc_id_ != nullptr;
}

bool CoreAudioDevice::IsStreamRunning() const noexcept {
    return is_running_;
}

void CoreAudioDevice::StopStream(bool /*wait_for_stop_stream*/) {
    XAMP_LOG_D(log_, "StopStream");
    if (is_running_) {
        is_running_ = false;
        std::unique_lock<FastMutex> lock{mutex_};
        stop_event_.wait(lock);
    }
    MSleep(std::chrono::milliseconds(10));
    CoreAudioThrowIfError(::AudioDeviceStop(device_id_, ioproc_id_));
    is_running_ = false;
}

void CoreAudioDevice::CloseStream() {
    XAMP_LOG_D(log_, "CloseStream");
    CoreAudioThrowIfError(::AudioDeviceStop(device_id_, ioproc_id_));
    CoreAudioThrowIfError(::AudioDeviceDestroyIOProcID(device_id_, ioproc_id_));
    ioproc_id_ = nullptr;
    ReleaseHogMode(device_id_);
}

void CoreAudioDevice::StartStream() {
    XAMP_LOG_D(log_, "StartStream!");
    CoreAudioThrowIfError(::AudioDeviceStart(device_id_, ioproc_id_));
    is_running_ = true;
}

void CoreAudioDevice::SetStreamTime(double stream_time) noexcept {
    stream_time_ = stream_time
                   * static_cast<double>(format_.GetAvgFramesPerSec());
}

double CoreAudioDevice::GetStreamTime() const noexcept {
    return stream_time_ / static_cast<double>(format_.GetAvgFramesPerSec());
}

uint32_t CoreAudioDevice::GetVolume() const {
    auto volume = SystemVolume(kAudioHardwareServiceDeviceProperty_VirtualMainVolume, device_id_)
                      .GetGain() * 100;
    return static_cast<uint32_t>(volume);
}

void CoreAudioDevice::SetVolume(uint32_t volume) const {
    SystemVolume(kAudioHardwareServiceDeviceProperty_VirtualMainVolume, device_id_)
        .SetGain(float(volume) / float(100.0));

    XAMP_LOG_D(log_, "Current volume: {}%", GetVolume());
}

void CoreAudioDevice::SetMute(bool mute) const {
    SystemVolume(kAudioDevicePropertyMute, device_id_).SetMuted(mute);
}

bool CoreAudioDevice::IsMuted() const {
    return SystemVolume(kAudioDevicePropertyMute, device_id_).IsMuted();
}

bool CoreAudioDevice::IsHardwareControlVolume() const {
    return SystemVolume(kAudioDevicePropertyMute, device_id_).HasProperty();
}

PackedFormat CoreAudioDevice::GetPackedFormat() const noexcept {
    return PackedFormat::INTERLEAVED;
}

uint32_t CoreAudioDevice::GetBufferSize() const noexcept {
    return buffer_size_;
}

void CoreAudioDevice::FillSamples(AudioBufferList *output_data, double sample_time, bool is_slient) {
    auto const buffer_count = output_data->mNumberBuffers;
    size_t num_filled_frames = 0;
    for (uint32_t i = 0; i < buffer_count; ++i) {
        const auto buffer = output_data->mBuffers[i];
        const uint32_t num_sample = static_cast<uint32_t>(buffer.mDataByteSize
                                                          / sizeof(float)
                                                          / format_.GetChannels());
        stream_time_ = stream_time_ + num_sample * 2;
        auto stream_time = stream_time_ / static_cast<double>(format_.GetAvgFramesPerSec());
        if (is_slient) {
            MemorySet(buffer.mData, 0, buffer.mDataByteSize);
            continue;
        }
        XAMP_LIKELY(callback_->OnGetSamples(static_cast<float*>(buffer.mData),
                                            num_sample,
                                            num_filled_frames,
                                            stream_time,
                                            sample_time) == DataCallbackResult::CONTINUE) {
            continue;
        } else {
            is_running_ = false;
            break;
        }
    }
}

OSStatus CoreAudioDevice::OnAudioDeviceIOProc(AudioDeviceID,
                                              AudioTimeStamp const*,
                                              AudioBufferList const*,
                                              AudioTimeStamp const*,
                                              AudioBufferList *output_data,
                                              AudioTimeStamp const* outputTimeStamp,
                                              void *user_data) {
    auto* device = static_cast<CoreAudioDevice*>(user_data);

    double sample_time = 0.0;
    if ((outputTimeStamp->mFlags & kAudioTimeStampHostTimeValid) == 0) {
        sample_time = static_cast<double>(GetUnixTime());
    } else {
        sample_time =
            outputTimeStamp->mSampleTime
            + device->latency_;
    }

    if (!device->is_running_) {
        std::unique_lock<FastMutex> lock{device->mutex_};
        device->stop_event_.notify_one();
        device->FillSamples(output_data, sample_time, true);
        XAMP_LOG_D(device->log_, "Stop request!");
        return noErr;
    }

    device->FillSamples(output_data, sample_time, false);
    return noErr;
}

void CoreAudioDevice::AbortStream() noexcept {
}

uint32_t CoreAudioDevice::GetHardwareLantency(AudioDeviceID device_id, AudioObjectPropertyScope scope) {
    AudioObjectPropertyAddress property_address = {
        kAudioDevicePropertyLatency,
        scope,
        kAudioObjectPropertyElementMaster
    };

    uint32_t latency = 0.0;
    uint32_t size = sizeof(latency);
    auto result = ::AudioObjectGetPropertyData(device_id,
                                               &property_address,
                                               0,
                                               nullptr,
                                               &size,
                                               &latency);
    CoreAudioThrowIfError(result);

    uint32_t safe_offet = 0;
    property_address.mSelector = kAudioDevicePropertySafetyOffset;
    result = ::AudioObjectGetPropertyData(device_id, &property_address,
                                          0,
                                          nullptr,
                                          &size,
                                          &safe_offet);
    CoreAudioThrowIfError(result);

    uint32_t stream_latency = 0;
    uint32_t numStreams;
    property_address.mSelector = kAudioDevicePropertyStreams;
    result = ::AudioObjectGetPropertyDataSize(device_id,
                                              &property_address,
                                              0,
                                              nullptr,
                                              &numStreams);

    std::vector<AudioStreamID> streams (numStreams);
    size = sizeof (AudioStreamID*);
    result = ::AudioObjectGetPropertyData(device_id,
                                          &property_address,
                                          0,
                                          nullptr,
                                          &size,
                                          streams.data());

    property_address.mSelector = kAudioStreamPropertyLatency;
    size = sizeof (stream_latency);
    // We could check all streams for the device,
    // but it only ever seems to return the stream latency on the first stream.
    result = ::AudioObjectGetPropertyData(streams[0],
                                          &property_address,
                                          0,
                                          nullptr,
                                          &size,
                                          &stream_latency);
    CoreAudioThrowIfError(result);

    XAMP_LOG_D(log_, "Device latency: {} us", latency);
    XAMP_LOG_D(log_, "Device offset: {} us", safe_offet);
    XAMP_LOG_D(log_, "Device stream latency: {} us", stream_latency);

    return latency + safe_offet + stream_latency;
}

}
