//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/logger.h>
#include <base/memory.h>
#include <base/uuidof.h>

#include <output_device/idevicetype.h>

XAMP_OUTPUT_DEVICE_POSIX_NAMESPACE_BEGIN

XAMP_DECLARE_LOG_NAME(PipeWireOutputDeviceType);

class PipeWireOutputDeviceType final : public IDeviceType {
	XAMP_DECLARE_MAKE_CLASS_UUID(PipeWireOutputDeviceType, "552B7894-0F20-4568-B66A-98E598CC0B34")

public:
	XAMP_DECLARE_UUID_CLASS_DESC(PipeWireOutputDeviceType, "PipeWire")

	PipeWireOutputDeviceType();

	~PipeWireOutputDeviceType() override = default;

	void ScanNewDevice() override;

	[[nodiscard]] size_t GetDeviceCount() const override;

	[[nodiscard]] DeviceInfo GetDeviceInfo(uint32_t device) const override;

	[[nodiscard]] std::vector<DeviceInfo> GetDeviceInfo() const override;

	[[nodiscard]] std::optional<DeviceInfo> GetDefaultDeviceInfo() const override;

	ScopedPtr<IOutputDevice> MakeDevice(const std::shared_ptr<xamp::base::IThreadPoolExecutor>& thread_pool,
		const std::string& device_id) override;

private:
	class PipeWireOutputDeviceTypeImpl;
	ScopedPtr<PipeWireOutputDeviceTypeImpl> impl_;
};

XAMP_OUTPUT_DEVICE_POSIX_NAMESPACE_END
