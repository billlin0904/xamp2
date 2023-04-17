//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/stream.h>
#include <stream/icddevice.h>

#include <base/pimplptr.h>

#ifdef XAMP_OS_WIN

XAMP_STREAM_NAMESPACE_BEGIN

class BassCDDevice final : public ICDDevice {
public:
	XAMP_PIMPL(BassCDDevice)

	explicit BassCDDevice(char driver_letter);

	void SetAction(CDDeviceAction action) override;

	void SetSpeed(uint32_t speed) override;

	void SetMaxSpeed() override;

	uint32_t GetSpeed() const override;

	bool DoorIsOpen() const override;

	CDDeviceInfo GetCDDeviceInfo() const override;

	void Release() override;

	CDText GetCDText() const override;

	double GetDuration(uint32_t track) const override;

	Vector<std::wstring> GetTotalTracks() const override;

	std::string GetISRC(uint32_t track) const override;
private:
	class BassCDDeviceImpl;
	PimplPtr<BassCDDeviceImpl> impl_;
};

XAMP_STREAM_NAMESPACE_END

#endif // XAMP_OS_WIN

