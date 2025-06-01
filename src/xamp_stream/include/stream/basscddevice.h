//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/stream.h>
#include <stream/icddevice.h>

#include <base/pimplptr.h>

XAMP_STREAM_NAMESPACE_BEGIN

#ifdef XAMP_OS_WIN

class BassCDDevice final : public ICDDevice {
public:
	XAMP_PIMPL(BassCDDevice)

	explicit BassCDDevice(char driver_letter);

	void SetAction(CDDeviceAction action) override;

	void SetSpeed(uint32_t speed) override;

	void SetMaxSpeed() override;

	[[nodiscard]] uint32_t GetSpeed() const override;

	[[nodiscard]] bool DoorIsOpen() const override;

	[[nodiscard]] CDDeviceInfo GetCDDeviceInfo() const override;

	void Release() override;

	[[nodiscard]] CDText GetCDText() const override;

	[[nodiscard]] double GetDuration(uint32_t track) const override;

	[[nodiscard]] std::vector<std::wstring> GetTotalTracks() const override;

	[[nodiscard]] std::string GetISRC(uint32_t track) const override;
private:
	class BassCDDeviceImpl;
	ScopedPtr<BassCDDeviceImpl> impl_;
};

#endif

XAMP_STREAM_NAMESPACE_END

