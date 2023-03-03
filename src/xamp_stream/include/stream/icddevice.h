//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/stream.h>

#include <base/stl.h>
#include <base/align_ptr.h>
#include <base/enum.h>

namespace xamp::stream {

XAMP_MAKE_ENUM(CDDeviceAction,
		CD_DOOR_CLOSE,
		CD_DOOR_OPEN,
		CD_DOOR_LOCK,
		CD_DOOR_UNLOCK)

struct XAMP_STREAM_API CDDeviceInfo {
	bool can_open{ false };
	bool can_lock{ false };
	bool can_read_cdtext{ false };
	uint32_t max_speed{ 0 };
	uint32_t cache_size{ 0 };
	std::wstring vendor;
	std::wstring product;
	std::wstring rev;
	std::wstring device_letter;
};

struct XAMP_STREAM_API CDText {
	std::wstring title;
	std::wstring performer;
	std::wstring song_writer;
	std::wstring composer;
	std::wstring arranger;
	std::wstring message;
	std::wstring genre;
	std::wstring isrc;
	std::wstring upc;
	std::wstring disc_id;
};

class XAMP_STREAM_API XAMP_NO_VTABLE ICDDevice {
public:
	XAMP_BASE_CLASS(ICDDevice)

	virtual void SetAction(CDDeviceAction action);

	virtual void SetSpeed(uint32_t speed) = 0;

	virtual void SetMaxSpeed() = 0;

	[[nodiscard]] virtual uint32_t GetSpeed() const = 0;

	[[nodiscard]] virtual bool DoorIsOpen() const = 0;

	[[nodiscard]] virtual CDDeviceInfo GetCDDeviceInfo() const = 0;

	[[nodiscard]] virtual CDText GetCDText() const = 0;

	[[nodiscard]] virtual Vector<std::wstring> GetTotalTracks() const = 0;

	virtual void Release() = 0;

	[[nodiscard]] virtual double GetDuration(uint32_t track) const = 0;

	[[nodiscard]] virtual std::string GetISRC(uint32_t track) const = 0;
protected:
	ICDDevice() = default;
};

}

