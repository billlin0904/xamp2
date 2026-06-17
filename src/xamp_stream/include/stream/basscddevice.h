//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/stream.h>
#include <stream/icddevice.h>

#include <base/memory.h>

XAMP_STREAM_NAMESPACE_BEGIN

#ifdef XAMP_OS_WIN

class BassCDDevice final : public ICDDevice {
public:
	XAMP_PIMPL(BassCDDevice)

	explicit BassCDDevice(char driver_letter);

	void setAction(CDDeviceAction action) override;

	void setSpeed(uint32_t speed) override;

	void setMaxSpeed() override;

	[[nodiscard]] uint32_t getSpeed() const override;

	[[nodiscard]] bool doorIsOpen() const override;

	[[nodiscard]] CDDeviceInfo getCDDeviceInfo() const override;

	void release() override;

	[[nodiscard]] CDText getCDText() const override;

	[[nodiscard]] double getDuration(uint32_t track) const override;

	[[nodiscard]] std::vector<std::wstring> getTotalTracks() const override;

	[[nodiscard]] std::string getISRC(uint32_t track) const override;
private:
	class BassCDDeviceImpl;
	ScopedPtr<BassCDDeviceImpl> impl_;
};

#endif

XAMP_STREAM_NAMESPACE_END
