#include <base/str_utilts.h>
#include <stream/basslib.h>
#include <stream/basscddevice.h>

#ifdef XAMP_OS_WIN

namespace xamp::stream {

static DWORD Drive2BassID(char driver_letter) {
	for (DWORD i = 0; i < 25; i++) {
		BASS_CD_INFO cdinfo{};
		if (BASS.CDLib->BASS_CD_GetInfo(i, &cdinfo)) {
			if (cdinfo.letter == static_cast<int>(driver_letter)) {
				return i;
			}
		}
	}
	throw DeviceNotFoundException();
}

class BassCDDevice::BassCDDeviceImpl {
public:
	explicit BassCDDeviceImpl(char driver_letter)
		: driver_(Drive2BassID(driver_letter)) {
	}

	~BassCDDeviceImpl() {
		Release();
	}

	void SetAction(CDDeviceAction action) {
		BassIfFailedThrow(BASS.CDLib->BASS_CD_Door(driver_, static_cast<DWORD>(action)));
	}

	void SetSpeed(uint32_t speed) {
		BassIfFailedThrow(BASS.CDLib->BASS_CD_SetSpeed(driver_, speed));
	}

	[[nodiscard]] uint32_t GetSpeed() const {
		return BASS.CDLib->BASS_CD_GetSpeed(driver_);
	}

	[[nodiscard]] bool DoorIsOpen() const {
		return BASS.CDLib->BASS_CD_DoorIsOpen(driver_);
	}

	[[nodiscard]] CDText GetCDText() const {
		CDText cd_text;
		auto const * text = BASS.CDLib->BASS_CD_GetID(driver_, BASS_CDID_TEXT);
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

	[[nodiscard]] std::vector<std::wstring> GetTotalTracks() const {
		std::vector<std::wstring> tracks;
		const auto num_track = BASS.CDLib->BASS_CD_GetTracks(driver_);
		if (num_track == kBassError) {
			return tracks;
		}

		for (uint32_t i = 0; i < num_track; ++i) {
			tracks.push_back(String::ToStdWString(String::StringPrint("Tacks%2u", i)));
		}
		return tracks;
	}

	[[nodiscard]] CDDeviceInfo GetCDDeviceInfo() const {
		BASS_CD_INFO info{};
		BassIfFailedThrow(BASS.CDLib->BASS_CD_GetInfo(driver_, &info));
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

	[[nodiscard]] std::wstring GetCDDB() const {
		auto const* text = BASS.CDLib->BASS_CD_GetID(driver_, BASS_CDID_TEXT);
		if (!text) {
			return L"";
		}
		return String::ToStdWString(text);
	}

	void Release() {
		BASS.CDLib->BASS_CD_Release(driver_);
	}
private:
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

CDDeviceInfo BassCDDevice::GetCDDeviceInfo() const {
	return impl_->GetCDDeviceInfo();
}

void BassCDDevice::Release() {
	impl_->Release();
}

std::wstring BassCDDevice::GetCDDB() const {
	return impl_->GetCDDB();
}

}

#endif