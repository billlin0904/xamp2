#include <vector>
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <mach/thread_policy.h>
#include <time.h>

#include <AudioToolbox/AudioToolbox.h>

#include <base/stl.h>
#include <base/logger.h>
#include <base/shared_singleton.h>
#include <base/memory.h>
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
    logger_ = XampLoggerFactory.getLogger(kCoreAudioLoggerName);
}

CoreAudioDevice::~CoreAudioDevice() {
    try { closeStream(); } catch (...) {}
    restoreIntegerStream();
}

void CoreAudioDevice::setBlance() {
    auto main_volume = SystemVolume(kAudioHardwareServiceDeviceProperty_VirtualMainVolume, device_id_);
    // kAudioHardwareUnknownPropertyError
    try {
        const auto blance = main_volume.getBlance();
        if (blance != 0.5f) {
            XAMP_LOG_D(logger_, "Device volume not blance: {}!", blance);
            main_volume.setBlance(0.5);
        }
    } catch (Exception const &e) {
        XAMP_LOG_D(logger_, "Failure to set volume blance, {}!", e.getErrorMessage());
    }
}

void CoreAudioDevice::openStream(AudioFormat const &output_format) {
    if (isStreamOpen()) closeStream();
    if (bitperfect_) {
        openIntegerStream(output_format);
        return;
    }
    if (io_format_ != DsdIoFormat::IO_FORMAT_PCM)
        throw std::runtime_error("CoreAudio DoP requires the BitPerfect integer playback path");
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
        XAMP_LOG_D(logger_, "Format is non interleaved.");
    } else {
        XAMP_LOG_D(logger_, "Format is interleaved.");
    }

    if (!IsSupportSampleRate(device_id_, output_format.getSampleRate())) {
        throw DeviceUnSupportedFormatException(output_format);
    }

    if (format_ != output_format) {
        fmt.mFormatID = kAudioFormatLinearPCM;
        fmt.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
        fmt.mSampleRate = output_format.getSampleRate();
        fmt.mChannelsPerFrame = output_format.getChannels();
        fmt.mFramesPerPacket = 1;
        fmt.mBitsPerChannel = output_format.getBitsPerSample();
        fmt.mBytesPerFrame = output_format.getBytesPerSample();
        fmt.mBytesPerPacket = output_format.getBytesPerSample();
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
        XAMP_LOG_D(logger_, "update audio format {}.", output_format);
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
    XAMP_LOG_D(logger_, "Allocate buffer size:{}.", buffer_size);

    UInt32 size = buffer_size;
    dataSize = sizeof(UInt32);
    audio_property_.mSelector = kAudioDevicePropertyBufferFrameSize;
    CoreAudioThrowIfError(::AudioObjectSetPropertyData(device_id_,
                                                       &audio_property_,
                                                       0,
                                                       nullptr,
                                                       dataSize,
                                                       &size));
    XAMP_LOG_D(logger_, "Set buffer size:{}.", size);

    buffer_size_ = output_format.getChannels() * buffer_size;

    latency_ = getHardwareLantency(device_id_, kAudioDevicePropertyScopeOutput);

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
        XAMP_LOG_D(logger_, "Set hog mode!");
        ReleaseHogMode(device_id_);
        SetHogMode(device_id_);
    }

    setBlance();
}

void CoreAudioDevice::setAudioCallback(IAudioCallback *callback) {
    callback_ = callback;
}

bool CoreAudioDevice::isStreamOpen() const {
    return ioproc_id_ != nullptr;
}

bool CoreAudioDevice::isStreamRunning() const {
    return is_running_;
}

void CoreAudioDevice::stopStream(bool /*wait_for_stop_stream*/) {
    is_running_ = false;
    if (ioproc_id_) CoreAudioThrowIfError(::AudioDeviceStop(device_id_, ioproc_id_));
}

void CoreAudioDevice::closeStream() {
    is_running_ = false;
    OSStatus stop_status = noErr;
    OSStatus destroy_status = noErr;
    if (ioproc_id_) {
        stop_status = ::AudioDeviceStop(device_id_, ioproc_id_);
        destroy_status = ::AudioDeviceDestroyIOProcID(device_id_, ioproc_id_);
        ioproc_id_ = nullptr;
    }
    restoreIntegerStream();
    if (is_hog_mode_ && !bitperfect_) ReleaseHogMode(device_id_);
    CoreAudioThrowIfError(stop_status);
    CoreAudioThrowIfError(destroy_status);
}

void CoreAudioDevice::startStream() {
    XAMP_LOG_D(logger_, "startStream!");
    is_running_ = true;
    const auto status = ::AudioDeviceStart(device_id_, ioproc_id_);
    if (status != noErr) is_running_ = false;
    CoreAudioThrowIfError(status);
}

void CoreAudioDevice::setStreamTime(double stream_time) {
    stream_time_ = stream_time
                   * static_cast<double>(format_.getAvgFramesPerSec());
}

double CoreAudioDevice::getStreamTime() const {
    return stream_time_ / static_cast<double>(format_.getAvgFramesPerSec());
}

uint32_t CoreAudioDevice::getVolume() const {
    if (bitperfect_) return 100;
    auto volume = SystemVolume(kAudioHardwareServiceDeviceProperty_VirtualMainVolume, device_id_)
                      .getGain() * 100;
    return static_cast<uint32_t>(volume);
}

void CoreAudioDevice::setVolume(uint32_t volume) const {
    if (bitperfect_) return;
    SystemVolume(kAudioHardwareServiceDeviceProperty_VirtualMainVolume, device_id_)
        .setGain(float(volume) / float(100.0));

    XAMP_LOG_D(logger_, "Current volume: {}%", getVolume());
}

void CoreAudioDevice::setMute(bool mute) const {
    if (bitperfect_) return;
    SystemVolume(kAudioDevicePropertyMute, device_id_).setMuted(mute);
}

bool CoreAudioDevice::isMuted() const {
    if (bitperfect_) return false;
    return SystemVolume(kAudioDevicePropertyMute, device_id_).isMuted();
}

bool CoreAudioDevice::isHardwareControlVolume() const {
    //return SystemVolume(kAudioDevicePropertyMute, device_id_).hasProperty();
    return false;
}

PackedFormat CoreAudioDevice::getPackedFormat() const {
    return PackedFormat::INTERLEAVED;
}

uint32_t CoreAudioDevice::getBufferSize() const {
    return buffer_size_;
}

void CoreAudioDevice::fillSamples(AudioBufferList *output_data, double sample_time, bool is_slient) {
    const auto buffer_count = output_data->mNumberBuffers;
    size_t num_filled_frames = 0;
    for (uint32_t i = 0; i < buffer_count; ++i) {
        const auto buffer = output_data->mBuffers[i];
        const uint32_t num_sample = static_cast<uint32_t>(buffer.mDataByteSize
                                                          / sizeof(float)
                                                          / format_.getChannels());
        stream_time_ = stream_time_ + num_sample * 2;
        auto stream_time = stream_time_ / static_cast<double>(format_.getAvgFramesPerSec());
        if (is_slient) {
            MemorySet(buffer.mData, 0, buffer.mDataByteSize);
            continue;
        }
        if (callback_->onGetSamples(static_cast<float*>(buffer.mData),
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
        sample_time = static_cast<double>(time(nullptr));
    } else {
        sample_time =
            outputTimeStamp->mSampleTime
            + device->latency_;
    }

    if (device->bitperfect_) {
        try {
            device->fillIntegerSamples(output_data, sample_time);
        } catch (const std::exception& error) {
            device->is_running_ = false;
            // No exception may escape the HAL callback. Supply valid DoP idle
            // on failure, then let the player dispatch its normal error event.
            try { device->fillIntegerSamples(output_data, sample_time); } catch (...) {}
            if (!device->integer_error_reported_) {
                device->integer_error_reported_ = true;
                try { if (device->callback_) device->callback_->onError(error); } catch (...) {}
            }
        } catch (...) {
            device->is_running_ = false;
            try { device->fillIntegerSamples(output_data, sample_time); } catch (...) {}
            if (!device->integer_error_reported_) {
                device->integer_error_reported_ = true;
                try {
                    if (device->callback_) device->callback_->onError(
                        std::runtime_error("CoreAudio integer output failed with an unknown error"));
                } catch (...) {}
            }
        }
        return noErr;
    }

    if (!device->is_running_) {
        std::unique_lock<FastMutex> lock{device->mutex_};
        device->stop_event_.notify_one();
        device->fillSamples(output_data, sample_time, true);
        XAMP_LOG_D(device->logger_, "stop request!");
        return noErr;
    }

    device->fillSamples(output_data, sample_time, false);
    return noErr;
}

void CoreAudioDevice::abortStream() {
}

void CoreAudioDevice::setVolumeLevelScalar(float level) {

}

uint32_t CoreAudioDevice::getHardwareLantency(AudioDeviceID device_id, AudioObjectPropertyScope scope) {
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
    UInt32 stream_bytes = 0;
    property_address.mSelector = kAudioDevicePropertyStreams;
    CoreAudioThrowIfError(::AudioObjectGetPropertyDataSize(device_id, &property_address,
        0, nullptr, &stream_bytes));
    if (!stream_bytes || stream_bytes % sizeof(AudioStreamID))
        throw std::runtime_error("CoreAudio device has no valid output streams");
    std::vector<AudioStreamID> streams(stream_bytes / sizeof(AudioStreamID));
    CoreAudioThrowIfError(::AudioObjectGetPropertyData(device_id, &property_address,
        0, nullptr, &stream_bytes, streams.data()));
    property_address = {kAudioStreamPropertyLatency, kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMain};
    size = sizeof(stream_latency);
    CoreAudioThrowIfError(::AudioObjectGetPropertyData(streams.front(), &property_address,
        0, nullptr, &size, &stream_latency));

    XAMP_LOG_D(logger_, "Device latency: {} us", latency);
    XAMP_LOG_D(logger_, "Device offset: {} us", safe_offet);
    XAMP_LOG_D(logger_, "Device stream latency: {} us", stream_latency);

    return latency + safe_offet + stream_latency;
}

}
