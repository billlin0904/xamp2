#include <output_device/osx/coreaudiodevicetype.h>
#include <output_device/osx/hogcoreaudiodevicetype.h>
#include <output_device/api.h>
#include <iostream>
#include <stdexcept>
using namespace xamp::output_device;
using namespace xamp::output_device::osx;
int main() {
    try {
        CoreAudioDeviceType shared;
        HogCoreAudioDeviceType exclusive;
        if (shared.getTypeId() == exclusive.getTypeId()) throw std::runtime_error("backend IDs must differ");
        // All enumeration APIs must retain the dynamic backend, including through a base pointer.
        for (CoreAudioDeviceType* backend : {&shared, static_cast<CoreAudioDeviceType*>(&exclusive)}) {
            backend->scanNewDevice();
            const auto devices = backend->getDeviceInfo();
            for (size_t i = 0; i < devices.size(); ++i) {
                if (isExclusiveDevice(devices[i]) != (backend == &exclusive))
                    throw std::runtime_error("exclusive classification does not match backend");
                if (devices[i].device_type_id != backend->getTypeId() ||
                    backend->getDeviceInfo(static_cast<uint32_t>(i)).device_type_id != backend->getTypeId())
                    throw std::runtime_error("enumeration returned a different backend ID");
            }
            if (auto device = backend->getDefaultDeviceInfo(); device && device->device_type_id != backend->getTypeId())
                throw std::runtime_error("default device lost its backend ID");
        }
        std::cout << "Shared and Hog Mode device identity tests passed (read-only enumeration)\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
