#include <output_device/devicefactory.h>
#include <output_device/output_device.h>

#include "xamp.h"

using namespace xamp::output_device;

Xamp::Xamp(QWidget *parent)
	: QMainWindow(parent) {
	ui.setupUi(this);
	InitialDevice();
	if (auto device = DeviceFactory::Instance().CreateDefaultDevice()) {
		device.value()->ScanNewDevice();
	}
}
