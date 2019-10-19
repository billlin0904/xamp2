#include "gtest/gtest.h"

#include <output_device/devicefactory.h>

using namespace xamp::output_device;

TEST(UnitTest, OpenDefaultDeviceTest) {
    if (auto default_device = DeviceFactory::Instance().CreateDefaultDevice()) {
        auto default_device_info = default_device.value()->GetDefaultDeviceInfo();
        EXPECT_TRUE(default_device_info.is_default_device);
    } else {
        EXPECT_TRUE(false);
    }
}

TEST(UnitTest, OpenStreamTest) {
    if (auto default_device = DeviceFactory::Instance().CreateDefaultDevice()) {
        auto default_device_info = default_device.value()->GetDefaultDeviceInfo();
        auto device = default_device.value()->MakeDevice(default_device_info.device_id);
        AudioFormat format(2, 16, 44100);
        device->OpenStream(format);
        EXPECT_TRUE(true);
    } else {
        EXPECT_TRUE(false);
    }
}

TEST(UnitTest, SetGetVolumeTest) {
    if (auto default_device = DeviceFactory::Instance().CreateDefaultDevice()) {
        auto default_device_info = default_device.value()->GetDefaultDeviceInfo();
        auto device = default_device.value()->MakeDevice(default_device_info.device_id);
        AudioFormat format(2, 16, 44100);
        device->OpenStream(format);
        auto volume = device->GetVolume();
        device->SetVolume(volume);
        EXPECT_TRUE(true);
    } else {
        EXPECT_TRUE(false);
    }
}
