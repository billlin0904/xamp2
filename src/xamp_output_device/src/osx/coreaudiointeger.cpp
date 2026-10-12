#include <output_device/osx/coreaudiodevice.h>
#include <output_device/osx/coreaudiointeger.h>
#include <output_device/osx/coreaudioexception.h>
#include <output_device/osx/osx_utitl.h>
#include <output_device/iaudiocallback.h>
#include <algorithm>
#include <thread>
#include <unistd.h>

namespace xamp::output_device::osx {
namespace {
constexpr AudioObjectPropertySelector watched_properties[] = {
    kAudioStreamPropertyPhysicalFormat, kAudioStreamPropertyVirtualFormat,
    kAudioDevicePropertyNominalSampleRate, kAudioDevicePropertyHogMode
};
AudioObjectPropertyAddress property(AudioObjectPropertySelector selector,
    AudioObjectPropertyScope scope = kAudioObjectPropertyScopeGlobal) {
    return {selector, scope, kAudioObjectPropertyElementMain};
}

template<class T> T read(AudioObjectID object, AudioObjectPropertySelector selector,
    AudioObjectPropertyScope scope = kAudioObjectPropertyScopeGlobal) {
    auto address = property(selector, scope);
    T value{};
    UInt32 bytes = sizeof(value);
    CoreAudioThrowIfError(AudioObjectGetPropertyData(object, &address, 0, nullptr, &bytes, &value));
    if (bytes != sizeof(value)) throw std::runtime_error("CoreAudio returned an invalid property size");
    return value;
}

template<class T> void write(AudioObjectID object, AudioObjectPropertySelector selector, const T& value) {
    auto address = property(selector);
    CoreAudioThrowIfError(AudioObjectSetPropertyData(object, &address, 0, nullptr, sizeof(value), &value));
}

template<class T> std::vector<T> readList(AudioObjectID object, AudioObjectPropertySelector selector,
    AudioObjectPropertyScope scope = kAudioObjectPropertyScopeGlobal) {
    auto address = property(selector, scope);
    UInt32 bytes = 0;
    CoreAudioThrowIfError(AudioObjectGetPropertyDataSize(object, &address, 0, nullptr, &bytes));
    if (!bytes || bytes % sizeof(T)) throw std::runtime_error("CoreAudio returned an empty or invalid format list");
    std::vector<T> values(bytes / sizeof(T));
    CoreAudioThrowIfError(AudioObjectGetPropertyData(object, &address, 0, nullptr, &bytes, values.data()));
    if (bytes % sizeof(T) || bytes / sizeof(T) > values.size())
        throw std::runtime_error("CoreAudio format list changed while reading");
    values.resize(bytes / sizeof(T));
    return values;
}

void setFormat(AudioStreamID stream, AudioObjectPropertySelector selector,
    const AudioStreamBasicDescription& format) {
    if (!integer_audio::sameFormat(read<AudioStreamBasicDescription>(stream, selector), format))
        write(stream, selector, format);
    // HAL format changes may complete asynchronously. Never start on an unverified format.
    for (int attempt = 0; attempt < 50; ++attempt) {
        if (integer_audio::sameFormat(read<AudioStreamBasicDescription>(stream, selector), format)) return;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    throw std::runtime_error("CoreAudio did not accept the exact integer stream format");
}
}

void CoreAudioDevice::setBitPerfect(bool enabled) {
    if (isStreamOpen()) throw std::runtime_error("Close CoreAudio stream before changing BitPerfect mode");
    if (enabled && !is_hog_mode_)
        throw std::runtime_error("BitPerfect/DoP requires CoreAudio (Hog Mode); select that output device");
    bitperfect_ = enabled;
}

void CoreAudioDevice::setIntegerPcmFormat(const xamp::pcm::Format& format) {
    if (isStreamOpen()) throw std::runtime_error("Close CoreAudio stream before changing integer format");
    if (!xamp::pcm::valid(format) || format.channels != 2 || format.container_bits != 32 ||
        format.layout != xamp::pcm::Layout::Interleaved ||
        format.byte_order != xamp::pcm::ByteOrder::Little || format.alignment != xamp::pcm::Alignment::Left)
        throw std::runtime_error("CoreAudio BitPerfect requires canonical stereo integer PCM");
    source_pcm_ = format;
}

void CoreAudioDevice::setIoFormat(DsdIoFormat format) {
    if (format == DsdIoFormat::IO_FORMAT_DSD)
        throw std::runtime_error("CoreAudio does not support Native DSD; select DoP");
    if (isStreamOpen()) throw std::runtime_error("Close CoreAudio stream before changing DSD transport");
    io_format_ = format;
}

DsdIoFormat CoreAudioDevice::getIoFormat() const { return io_format_; }

void CoreAudioDevice::openIntegerStream(const AudioFormat& format) {
    if (!is_hog_mode_ || !xamp::pcm::valid(source_pcm_) || source_pcm_.channels != 2 ||
        source_pcm_.sample_rate != format.getSampleRate() || format.getChannels() != 2 ||
        (io_format_ == DsdIoFormat::IO_FORMAT_DOP && source_pcm_.valid_bits != 24))
        throw std::runtime_error("CoreAudio BitPerfect/DoP has an incompatible source format");
    try {
        const auto streams = readList<AudioStreamID>(device_id_, kAudioDevicePropertyStreams,
            kAudioDevicePropertyScopeOutput);
        if (streams.size() != 1)
            throw std::runtime_error("CoreAudio BitPerfect/DoP currently requires one interleaved stereo output stream");
        integer_stream_ = streams.front();
        const auto owner = read<pid_t>(device_id_, kAudioDevicePropertyHogMode);
        if (owner != -1 && owner != getpid())
            throw std::runtime_error("CoreAudio device is in use by another exclusive player");
        if (owner != getpid()) {
            write(device_id_, kAudioDevicePropertyHogMode, getpid());
            owns_hog_ = true;
        }
        if (read<pid_t>(device_id_, kAudioDevicePropertyHogMode) != getpid())
            throw std::runtime_error("CoreAudio could not acquire exclusive access");

        original_rate_ = read<Float64>(device_id_, kAudioDevicePropertyNominalSampleRate);
        original_physical_ = read<AudioStreamBasicDescription>(integer_stream_, kAudioStreamPropertyPhysicalFormat);
        original_virtual_ = read<AudioStreamBasicDescription>(integer_stream_, kAudioStreamPropertyVirtualFormat);
        // Save before any mutation: changing mixing/rate can also change stream formats.
        restore_formats_ = restore_rate_ = true;
        auto mixing = property(kAudioDevicePropertySupportsMixing);
        if (AudioObjectHasProperty(device_id_, &mixing)) {
            original_mixing_ = read<UInt32>(device_id_, kAudioDevicePropertySupportsMixing);
            if (original_mixing_) {
                restore_mixing_ = true;
                write(device_id_, kAudioDevicePropertySupportsMixing, UInt32{0});
                if (read<UInt32>(device_id_, kAudioDevicePropertySupportsMixing) != 0)
                    throw std::runtime_error("CoreAudio could not disable device mixing for integer output");
            }
        }
        const auto candidates = readList<AudioStreamRangedDescription>(integer_stream_,
            kAudioStreamPropertyAvailablePhysicalFormats);
        const auto virtual_formats = readList<AudioStreamRangedDescription>(integer_stream_,
            kAudioStreamPropertyAvailableVirtualFormats);
        const auto selected = integer_audio::selectFormat(candidates, virtual_formats, source_pcm_);
        if (!selected)
            throw std::runtime_error("CoreAudio DAC does not expose a lossless stereo integer format at the requested carrier rate; lower the DSD rate or select another DAC");

        write(device_id_, kAudioDevicePropertyNominalSampleRate, static_cast<Float64>(source_pcm_.sample_rate));
        setFormat(integer_stream_, kAudioStreamPropertyPhysicalFormat, *selected);
        // No float virtual format or HAL conversion is permitted for strict output.
        setFormat(integer_stream_, kAudioStreamPropertyVirtualFormat, *selected);
        if (!integer_audio::sameFormat(read<AudioStreamBasicDescription>(integer_stream_,
                kAudioStreamPropertyPhysicalFormat), *selected) ||
            read<Float64>(device_id_, kAudioDevicePropertyNominalSampleRate) != source_pcm_.sample_rate)
            throw std::runtime_error("CoreAudio changed the requested physical format or sample rate");
        device_pcm_ = *integer_audio::pcmFormat(*selected);
        negotiated_format_ = *selected;
        const auto frames = read<UInt32>(device_id_, kAudioDevicePropertyBufferFrameSize,
            kAudioDevicePropertyScopeOutput);
        if (!frames || frames > 1024 * 1024)
            throw std::runtime_error("CoreAudio returned an invalid buffer size");
        pcm_buffer_.resize(static_cast<size_t>(frames) * source_pcm_.frameBytes());
        buffer_size_ = frames * source_pcm_.channels;
        format_ = format;
        dop_marker_ = 0x05;
        latency_ = getHardwareLantency(device_id_, kAudioDevicePropertyScopeOutput);
        integer_format_changed_ = false;
        integer_error_reported_ = false;
        for (unsigned i = 0; i < std::size(watched_properties); ++i) {
            auto address = property(watched_properties[i]);
            CoreAudioThrowIfError(AudioObjectAddPropertyListener(i < 2 ? integer_stream_ : device_id_,
                &address, OnIntegerPropertyChanged, this));
            ++integer_listener_count_;
        }
        // Recheck after installing listeners to close the negotiation/listener race.
        if (integer_format_changed_ || read<pid_t>(device_id_, kAudioDevicePropertyHogMode) != getpid() ||
            read<Float64>(device_id_, kAudioDevicePropertyNominalSampleRate) != source_pcm_.sample_rate ||
            !integer_audio::sameFormat(read<AudioStreamBasicDescription>(integer_stream_, kAudioStreamPropertyPhysicalFormat), *selected) ||
            !integer_audio::sameFormat(read<AudioStreamBasicDescription>(integer_stream_, kAudioStreamPropertyVirtualFormat), *selected))
            throw std::runtime_error("CoreAudio configuration changed while opening integer output");
        CoreAudioThrowIfError(AudioDeviceCreateIOProcID(device_id_, OnAudioDeviceIOProc, this, &ioproc_id_));
    } catch (...) {
        restoreIntegerStream();
        throw;
    }
}

OSStatus CoreAudioDevice::OnIntegerPropertyChanged(AudioObjectID, UInt32,
    const AudioObjectPropertyAddress*, void* user_data) {
    auto* device = static_cast<CoreAudioDevice*>(user_data);
    // This is a HAL property notification, not the real-time render callback.
    // A delayed notification for our own successful negotiation is harmless.
    try {
        if (read<pid_t>(device->device_id_, kAudioDevicePropertyHogMode) != getpid() ||
            read<Float64>(device->device_id_, kAudioDevicePropertyNominalSampleRate) != device->source_pcm_.sample_rate ||
            !integer_audio::sameFormat(read<AudioStreamBasicDescription>(device->integer_stream_,
                kAudioStreamPropertyPhysicalFormat), device->negotiated_format_) ||
            !integer_audio::sameFormat(read<AudioStreamBasicDescription>(device->integer_stream_,
                kAudioStreamPropertyVirtualFormat), device->negotiated_format_))
            device->integer_format_changed_ = true;
    } catch (...) {
        device->integer_format_changed_ = true;
    }
    return noErr;
}

void CoreAudioDevice::removeIntegerListeners() noexcept {
    while (integer_listener_count_) {
        const auto i = --integer_listener_count_;
        auto address = property(watched_properties[i]);
        AudioObjectRemovePropertyListener(i < 2 ? integer_stream_ : device_id_,
            &address, OnIntegerPropertyChanged, this);
    }
}

void CoreAudioDevice::restoreIntegerStream() noexcept {
    removeIntegerListeners();
    // Attempt every restoration even if the device was disconnected midway.
    auto restore = [this](auto action) {
        try { action(); }
        catch (const std::exception& e) { XAMP_LOG_E(logger_, "CoreAudio restore failed: {}", e.what()); }
        catch (...) {}
    };
    if (restore_mixing_) restore([&] { write(device_id_, kAudioDevicePropertySupportsMixing, original_mixing_); });
    if (restore_rate_) restore([&] { write(device_id_, kAudioDevicePropertyNominalSampleRate, original_rate_); });
    if (restore_formats_) {
        restore([&] { setFormat(integer_stream_, kAudioStreamPropertyPhysicalFormat, original_physical_); });
        restore([&] { setFormat(integer_stream_, kAudioStreamPropertyVirtualFormat, original_virtual_); });
    }
    restore_formats_ = restore_rate_ = restore_mixing_ = false;
    if (owns_hog_) restore([&] {
        if (read<pid_t>(device_id_, kAudioDevicePropertyHogMode) == getpid())
            write(device_id_, kAudioDevicePropertyHogMode, pid_t{-1});
    });
    owns_hog_ = false;
    integer_stream_ = kAudioObjectUnknown;
}

void CoreAudioDevice::fillIntegerSamples(AudioBufferList* output, double sample_time) {
    for (UInt32 i = 0; i < output->mNumberBuffers; ++i)
        if (output->mBuffers[i].mData) std::memset(output->mBuffers[i].mData, 0, output->mBuffers[i].mDataByteSize);
    if (integer_format_changed_)
        throw std::runtime_error("CoreAudio device format or exclusive ownership changed; reopen the device");
    if (output->mNumberBuffers != 1 || output->mBuffers[0].mNumberChannels != 2)
        throw std::runtime_error("CoreAudio changed the negotiated stereo buffer layout");
    auto& out = output->mBuffers[0];
    if (!out.mData || !out.mDataByteSize) return;
    if (!device_pcm_.frameBytes() || out.mDataByteSize % device_pcm_.frameBytes())
        throw std::runtime_error("CoreAudio returned a misaligned integer buffer");
    const size_t frames = out.mDataByteSize / device_pcm_.frameBytes();
    const size_t bytes = frames * source_pcm_.frameBytes();
    if (bytes > pcm_buffer_.size())
        throw std::runtime_error("CoreAudio increased the integer buffer size; reopen the device");
    std::memset(pcm_buffer_.data(), 0, bytes);
    size_t filled = 0;
    auto result = DataCallbackResult::CONTINUE;
    if (is_running_ && callback_) {
        result = callback_->onGetSamples(pcm_buffer_.data(), frames, filled,
            getStreamTime(), sample_time);
        if (filled > frames) throw std::runtime_error("CoreAudio integer callback exceeded its buffer");
    }
    if (io_format_ == DsdIoFormat::IO_FORMAT_DOP)
        integer_audio::frameDop({pcm_buffer_.data(), bytes}, filled, dop_marker_);
    std::span<std::byte> destination{static_cast<std::byte*>(out.mData), out.mDataByteSize};
    if (!xamp::pcm::convert({{pcm_buffer_.data(), bytes}, frames, source_pcm_},
        device_pcm_, {&destination, 1}))
        throw std::runtime_error("CoreAudio cannot losslessly pack the integer output");
    stream_time_ = stream_time_ + filled * source_pcm_.channels;
    if (result == DataCallbackResult::STOP) {
        is_running_ = false;
        // The strict player returns STOP on the callback after the final audio block.
        callback_->onPlaybackEnd();
    }
}
}
