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

	explicit BassCDDevice(char driver_letter);

	void SetAction(CDDeviceAction action) override;

	void SetSpeed(uint32_t speed) override;

	uint32_t GetSpeed() const override;

	bool DoorIsOpen() const override;

	CDDeviceInfo GetCDDeviceInfo() const override;

	void Release() override;

	CDText GetCDText() const override;

	uint32_t GetTrackLength(uint32_t track) const override;

	std::vector<std::wstring> GetTotalTracks() const override;
private:
	class BassCDDeviceImpl;
	AlignPtr<BassCDDeviceImpl> impl_;
};

}

#endif

