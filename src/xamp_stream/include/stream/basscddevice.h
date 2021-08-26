//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/stream.h>
#include <stream/cddevice.h>

#ifdef XAMP_OS_WIN

namespace xamp::stream {

class XAMP_STREAM_API BassCDDevice final : public CDDevice {
public:
	XAMP_PIMPL(BassCDDevice)

	explicit BassCDDevice(uint32_t driver);

	void SetAction(CDDeviceAction action) override;

	void SetSpeed(uint32_t speed) override;

	uint32_t GetSpeed() const override;

	bool DoorIsOpen() const override;

	CDDeviceInfo GetCDDeviceInfo() const override;

	void Release() override;

	CDText GetCDText() const override;

	std::vector<std::wstring> GetTotalTracks() const override;

	std::wstring GetCDDB() const override;
private:
	class BassCDDeviceImpl;
	AlignPtr<BassCDDeviceImpl> impl_;
};

}

#endif

