#include <output_device/osx/coreaudiodevice.h>
#include <output_device/osx/coreaudiointeger.h>
#include <output_device/iaudiocallback.h>
#include <array>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <unistd.h>

using namespace xamp::output_device;
using namespace xamp::output_device::osx;
namespace {
void require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
AudioStreamBasicDescription floatFormat() {
    return {48000, kAudioFormatLinearPCM, kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked, 8, 1, 8, 2, 32, 0};
}
struct Listener { AudioObjectPropertyListenerProc proc; void* data; AudioObjectID object; AudioObjectPropertySelector selector; };
struct Hal {
    pid_t owner{-1};
    UInt32 mixing{1};
    double rate{48000};
    AudioStreamBasicDescription physical{floatFormat()}, virt{floatFormat()};
    bool reject_virtual{false}, reject_create{false}, reject_hog{false};
    bool bad_rate{false}, multi_stream{false};
    unsigned writes{0}, creates{0}, destroys{0};
    std::vector<Listener> listeners;
    AudioDeviceIOProc proc{};
    void* user{};
} hal;

template<class T> OSStatus result(const T& value, UInt32* size, void* data) {
    if (*size < sizeof(value)) return kAudioHardwareBadPropertySizeError;
    *size = sizeof(value); std::memcpy(data, &value, sizeof(value)); return noErr;
}

struct Callback : IAudioCallback {
    unsigned calls{0}, ended{0}, errors{0};
    bool underflow{false};
    DataCallbackResult onGetSamples(void* data, size_t frames, size_t& filled, double, double) override {
        ++calls;
        if (underflow) throw std::runtime_error("test underflow");
        if (calls > 1) { filled = 0; return DataCallbackResult::STOP; }
        filled = frames - 1; // exercise a partial EOF buffer
        auto* bytes = static_cast<std::byte*>(data);
        for (size_t i = 0; i < filled * 2; ++i) { bytes[i * 4 + 1] = std::byte{0x12}; bytes[i * 4 + 2] = std::byte{0x34}; }
        return DataCallbackResult::CONTINUE;
    }
    void onPlaybackEnd() override { ++ended; }
    void onError(const std::exception&) override { ++errors; }
    void onVolumeChange(int32_t) override {}
    void onGlitch(std::chrono::milliseconds, uint32_t) override {}
};

AudioFormat source() { return AudioFormat(DataFormat::FORMAT_PCM, 2, ByteFormat::SINT32, 176400); }
void prepare(CoreAudioDevice& device, Callback& callback) {
    device.setBitPerfect(true);
    device.setIntegerPcmFormat(xamp::pcm::canonical(24, 2, 176400));
    device.setIoFormat(DsdIoFormat::IO_FORMAT_DOP);
    device.setAudioCallback(&callback);
}
void restored() {
    require(hal.owner == -1 && hal.mixing == 1 && hal.rate == 48000, "hog, mixing and sample rate restored");
    require(integer_audio::sameFormat(hal.physical, floatFormat()) && integer_audio::sameFormat(hal.virt, floatFormat()), "both stream formats restored");
    require(hal.listeners.empty(), "property listeners removed");
}
std::array<std::byte, 18> render() {
    std::array<std::byte, 18> data{};
    AudioBufferList buffers{}; buffers.mNumberBuffers = 1; buffers.mBuffers[0] = {2, static_cast<UInt32>(data.size()), data.data()};
    AudioTimeStamp stamp{}; stamp.mFlags = kAudioTimeStampHostTimeValid;
    require(hal.proc(1, &stamp, &buffers, &stamp, &buffers, &stamp, hal.user) == noErr, "HAL callback must not throw");
    return data;
}
}

extern "C" OSStatus MockAudioObjectGetPropertyDataSize(AudioObjectID, const AudioObjectPropertyAddress* p, UInt32, const void*, UInt32* size) {
    *size = p->mSelector == kAudioDevicePropertyStreams ? sizeof(AudioStreamID) * (hal.multi_stream ? 2 : 1) : sizeof(AudioStreamRangedDescription);
    return noErr;
}
extern "C" OSStatus MockAudioObjectGetPropertyData(AudioObjectID, const AudioObjectPropertyAddress* p, UInt32, const void*, UInt32* size, void* data) {
    switch (p->mSelector) {
    case kAudioDevicePropertyHogMode: return result(hal.owner, size, data);
    case kAudioDevicePropertySupportsMixing: return result(hal.mixing, size, data);
    case kAudioDevicePropertyNominalSampleRate: return result(hal.rate, size, data);
    case kAudioStreamPropertyPhysicalFormat: return result(hal.physical, size, data);
    case kAudioStreamPropertyVirtualFormat: return result(hal.virt, size, data);
    case kAudioDevicePropertyStreams: {
        if (hal.multi_stream) return result(std::array<AudioStreamID, 2>{2, 3}, size, data);
        return result(AudioStreamID{2}, size, data);
    }
    case kAudioStreamPropertyAvailableVirtualFormats:
    case kAudioStreamPropertyAvailablePhysicalFormats: {
        AudioStreamRangedDescription f{};
        f.mFormat = {176400, kAudioFormatLinearPCM, kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked, 6, 1, 6, 2, 24, 0};
        f.mSampleRateRange = hal.bad_rate ? AudioValueRange{44100, 48000} : AudioValueRange{176400, 176400};
        return result(f, size, data);
    }
    case kAudioDevicePropertyBufferFrameSize: return result(UInt32{3}, size, data);
    case kAudioDevicePropertyLatency:
    case kAudioDevicePropertySafetyOffset:
    return result(UInt32{0}, size, data);
    default: return kAudioHardwareUnknownPropertyError;
    }
}
extern "C" OSStatus MockAudioObjectSetPropertyData(AudioObjectID, const AudioObjectPropertyAddress* p, UInt32, const void*, UInt32, const void* data) {
    ++hal.writes;
    switch (p->mSelector) {
    case kAudioDevicePropertyHogMode:
        if (hal.reject_hog) return kAudioHardwareIllegalOperationError;
        hal.owner = *static_cast<const pid_t*>(data); break;
    case kAudioDevicePropertySupportsMixing: hal.mixing = *static_cast<const UInt32*>(data); break;
    case kAudioDevicePropertyNominalSampleRate: hal.rate = *static_cast<const double*>(data); break;
    case kAudioStreamPropertyPhysicalFormat: hal.physical = *static_cast<const AudioStreamBasicDescription*>(data); break;
    case kAudioStreamPropertyVirtualFormat:
        if (hal.reject_virtual && static_cast<const AudioStreamBasicDescription*>(data)->mFormatFlags & kAudioFormatFlagIsSignedInteger)
            return kAudioHardwareUnsupportedOperationError;
        hal.virt = *static_cast<const AudioStreamBasicDescription*>(data); break;
    default: return kAudioHardwareUnknownPropertyError;
    }
    return noErr;
}
extern "C" Boolean MockAudioObjectHasProperty(AudioObjectID, const AudioObjectPropertyAddress*) { return true; }
extern "C" OSStatus MockAudioObjectAddPropertyListener(AudioObjectID object, const AudioObjectPropertyAddress* p, AudioObjectPropertyListenerProc proc, void* data) {
    hal.listeners.push_back({proc, data, object, p->mSelector}); return noErr;
}
extern "C" OSStatus MockAudioObjectRemovePropertyListener(AudioObjectID object, const AudioObjectPropertyAddress* p, AudioObjectPropertyListenerProc, void*) {
    std::erase_if(hal.listeners, [&](const auto& l) { return l.object == object && l.selector == p->mSelector; }); return noErr;
}
extern "C" OSStatus MockAudioDeviceCreateIOProcID(AudioObjectID, AudioDeviceIOProc proc, void* user, AudioDeviceIOProcID* id) {
    if (hal.reject_create) return kAudioHardwareIllegalOperationError;
    hal.proc = proc; hal.user = user; *id = reinterpret_cast<AudioDeviceIOProcID>(uintptr_t{1}); ++hal.creates; return noErr;
}
extern "C" OSStatus MockAudioDeviceDestroyIOProcID(AudioObjectID, AudioDeviceIOProcID) { ++hal.destroys; return noErr; }
extern "C" OSStatus MockAudioDeviceStart(AudioObjectID, AudioDeviceIOProcID) { return noErr; }
extern "C" OSStatus MockAudioDeviceStop(AudioObjectID, AudioDeviceIOProcID) { return noErr; }

int main() {
    try {
        for (int failure = 0; failure < 6; ++failure) {
            hal = {};
            hal.reject_virtual = failure == 0; hal.reject_create = failure == 1;
            hal.reject_hog = failure == 2; hal.bad_rate = failure == 3;
            hal.multi_stream = failure == 4;
            if (failure == 5) hal.owner = getpid() + 1;
            Callback callback;
            { CoreAudioDevice device(1, true); prepare(device, callback);
              bool rejected = false; try { device.openStream(source()); } catch (const std::exception&) { rejected = true; }
              require(rejected && !device.isStreamOpen(), "unsupported device must fail closed");
              if (failure == 5) { require(hal.writes == 0, "never modify another owner's device"); hal.owner = -1; }
              restored(); }
        }
        hal = {};
        { CoreAudioDevice shared(1, false); bool rejected = false;
          try { shared.setBitPerfect(true); } catch (const std::exception&) { rejected = true; }
          require(rejected && hal.writes == 0, "shared device rejected without mutation"); }
        hal = {};
        Callback callback;
        { CoreAudioDevice device(1, true); prepare(device, callback); device.openStream(source());
          require(hal.owner == getpid() && hal.mixing == 0 && hal.rate == 176400, "exclusive carrier configuration applied");
          const auto notification = hal.listeners.front();
          AudioObjectPropertyAddress unchanged{notification.selector, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain};
          notification.proc(notification.object, 1, &unchanged, notification.data);
          auto idle = render(); require(callback.calls == 0 && idle[0] == std::byte{0x69}, "priming does not consume source");
          device.startStream(); auto data = render();
          require(data[0] == std::byte{0x12} && data[1] == std::byte{0x34}, "callback data reaches HAL unchanged");
          require(data[2] == std::byte{0xfa} && data[5] == std::byte{0xfa}, "DoP phase crosses odd priming buffer");
          require(data[12] == std::byte{0x69} && callback.ended == 0, "partial final block is padded, not ended early");
          render(); require(callback.ended == 1, "EOF delivered after final audio callback");
          render(); require(callback.ended == 1, "EOF event is emitted once");
          device.closeStream(); restored(); device.closeStream(); require(hal.destroys == 1, "close is idempotent"); }
        hal = {}; callback = {};
        { CoreAudioDevice device(1, true); prepare(device, callback); device.openStream(source()); device.startStream();
          callback.underflow = true; const auto idle = render();
          require(callback.errors == 1 && !device.isStreamRunning(), "underflow dispatches error and stops");
          require(idle[0] == std::byte{0x69} && idle[2] == std::byte{5}, "underflow supplies valid DoP idle");
          device.closeStream(); restored(); }
        hal = {}; callback = {};
        { CoreAudioDevice device(1, true); prepare(device, callback); device.openStream(source()); device.startStream();
          hal.physical.mSampleRate = 192000;
          auto listener = hal.listeners.front(); AudioObjectPropertyAddress changed{listener.selector, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain}; listener.proc(listener.object, 1, &changed, listener.data);
          render(); require(callback.calls == 0 && callback.errors == 1, "format change stops before consuming PCM");
          render(); require(callback.errors == 1, "format failure reports once while waiting for player shutdown");
          device.closeStream(); restored(); }
        std::cout << "CoreAudio fake-HAL negotiation, rollback, render and lifecycle tests passed\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
