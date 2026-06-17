#include "gtest/gtest.h"

#include <output_device/devicefactory.h>

using namespace xamp::output_device;

TEST(UnitTest, OpenDefaultDeviceTest) {
    if (auto default_device = DeviceFactory::Instance().CreateDefaultDevice()) {
        auto default_device_info = default_device.value()->getDefaultDeviceInfo();
        EXPECT_TRUE(default_device_info.is_default_device);
    } else {
        EXPECT_TRUE(false);
    }
}

TEST(UnitTest, OpenStreamTest) {
    if (auto default_device = DeviceFactory::Instance().CreateDefaultDevice()) {
        auto default_device_info = default_device.value()->getDefaultDeviceInfo();
        auto device = default_device.value()->makeDevice(default_device_info.device_id);
        AudioFormat format(2, 16, 44100);
        device->openStream(format);
        EXPECT_TRUE(true);
    } else {
        EXPECT_TRUE(false);
    }
}

TEST(UnitTest, SetGetVolumeTest) {
    if (auto default_device = DeviceFactory::Instance().CreateDefaultDevice()) {
        auto default_device_info = default_device.value()->getDefaultDeviceInfo();
        auto device = default_device.value()->makeDevice(default_device_info.device_id);
        AudioFormat format(2, 16, 44100);
        device->openStream(format);
        auto volume = device->getVolume();
        device->setVolume(volume);
        EXPECT_TRUE(true);
    } else {
        EXPECT_TRUE(false);
    }
}
