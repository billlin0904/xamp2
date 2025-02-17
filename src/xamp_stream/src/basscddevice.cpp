#include <stream/basscddevice.h>

#ifdef XAMP_OS_WIN

#include <stream/basslib.h>
#include <base/str_utilts.h>

namespace xamp::stream {

inline constexpr auto kCDSpeedMultiplier = 176.4;

namespace {
	DWORD Drive2BassID(char driver_letter) {
		for (DWORD i = 0; i < 25; i++) {
			BASS_CD_INFO cdinfo{};
			if (BASS_LIB.CDLib->BASS_CD_GetInfo(i, &cdinfo)) {
				char letter = 'A' + cdinfo.letter;
				if (letter == driver_letter) {
					return i;
				}
			}
		}
		throw DeviceNotFoundException();
	}
}

class BassCDDevice::BassCDDeviceImpl {
public:
	explicit BassCDDeviceImpl(char driver_letter)
		: driver_letter_(driver_letter)
		, driver_(Drive2BassID(driver_letter)) {
	}

	~BassCDDeviceImpl() {
		Release();
	}

	void SetAction(CDDeviceAction action) {
		BassIfFailedThrow(BASS_LIB.CDLib->BASS_CD_Door(driver_, static_cast<DWORD>(action)));
	}

	void SetSpeed(uint32_t speed) {
		BassIfFailedThrow(BASS_LIB.CDLib->BASS_CD_SetSpeed(driver_, speed));
	}

	XAMP_NO_DISCARD uint32_t GetSpeed() const {
		return static_cast<uint32_t>((BASS_LIB.CDLib->BASS_CD_GetSpeed(driver_) / kCDSpeedMultiplier));
	}

	XAMP_NO_DISCARD bool DoorIsOpen() const {
		return BASS_LIB.CDLib->BASS_CD_DoorIsOpen(driver_);
	}

	std::string GetISRC(uint32_t track) const {
		auto const* text = BASS_LIB.CDLib->BASS_CD_GetID(driver_, BASS_CDID_ISRC + track);
		if (!text) {
			return "";
		}
		return text;
	}

	XAMP_NO_DISCARD CDText GetCDText() const {
		CDText cd_text;
		auto const * text = BASS_LIB.CDLib->BASS_CD_GetID(driver_, BASS_CDID_TEXT);
		if (!text) {
			return cd_text;
		}
		while (*text) {
			auto tag = String::Split(text, "=");
			if (tag[0] == "TITLE") {
				cd_text.title = String::ToStdWString(tag[1].data());
			}
			text += strlen(text) + 1;
		}
		return cd_text;
	}

	XAMP_NO_DISCARD std::vector<std::wstring> GetTotalTracks() const {
		std::vector<std::wstring> tracks;
		const auto num_track = BASS_LIB.CDLib->BASS_CD_GetTracks(driver_);
		if (num_track == kBassError) {
			return tracks;
		}

		for (uint32_t i = 0; i < num_track; ++i) {
			std::wstringstream ostr;
			ostr << driver_letter_ << ":\\" << L"Track" << std::setfill(L'0') << std::setw(2) << i + 1 << L".cda";
			tracks.push_back(ostr.str());
		}
		return tracks;
	}

	XAMP_NO_DISCARD CDDeviceInfo GetCDDeviceInfo() const {
		BASS_CD_INFO info{};
		BassIfFailedThrow(BASS_LIB.CDLib->BASS_CD_GetInfo(driver_, &info));
		CDDeviceInfo device_info;
		device_info.can_lock = info.canlock;
		device_info.can_open = info.canopen;
		device_info.device_letter = std::to_wstring(info.letter);
		device_info.product = String::ToStdWString(info.product);
		device_info.vendor = String::ToStdWString(info.vendor);
		device_info.rev = String::ToStdWString(info.rev);
		device_info.cache_size = info.cache;
		device_info.max_speed = info.maxspeed;
		device_info.can_read_cdtext = info.cdtext;
		return device_info;
	}

	uint32_t GetTrackLength(uint32_t track) const {
		return BASS_LIB.CDLib->BASS_CD_GetTrackLength(driver_, track);
	}

	void Release() {
		BASS_LIB.CDLib->BASS_CD_Release(driver_);
	}

	void SetMaxSpeed() {
		// -1 = optimal performace.
		BassIfFailedThrow(BASS_LIB.CDLib->BASS_CD_SetSpeed(driver_, -1));
	}

	double GetDuration(uint32_t track) const {
		constexpr double kCDBytesPerSecond = 176400;
		return BASS_LIB.CDLib->BASS_CD_GetTrackLength(driver_, track) / kCDBytesPerSecond;
	}
private:
	char driver_letter_;
	DWORD driver_;
};

BassCDDevice::BassCDDevice(char driver_letter)
	: impl_(MakeAlign<BassCDDeviceImpl>(driver_letter)) {
}

XAMP_PIMPL_IMPL(BassCDDevice)

void BassCDDevice::SetAction(CDDeviceAction action) {
	impl_->SetAction(action);
}

void BassCDDevice::SetSpeed(uint32_t speed) {
	impl_->SetSpeed(speed);
}

void BassCDDevice::SetMaxSpeed() {
	impl_->SetMaxSpeed();
}

uint32_t BassCDDevice::GetSpeed() const {
	return impl_->GetSpeed();
}

bool BassCDDevice::DoorIsOpen() const {
	return impl_->DoorIsOpen();
}

CDText BassCDDevice::GetCDText() const {
	return impl_->GetCDText();
}

std::vector<std::wstring> BassCDDevice::GetTotalTracks() const {
	return impl_->GetTotalTracks();
}

double BassCDDevice::GetDuration(uint32_t track) const {
	return impl_->GetDuration(track);
}

std::string BassCDDevice::GetISRC(uint32_t track) const {
	return impl_->GetISRC(track);
}

CDDeviceInfo BassCDDevice::GetCDDeviceInfo() const {
	return impl_->GetCDDeviceInfo();
}

void BassCDDevice::Release() {
	impl_->Release();
}

}

#endif