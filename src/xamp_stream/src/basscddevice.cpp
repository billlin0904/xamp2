#include <stream/basscddevice.h>

#include <stream/basslib.h>
#include <base/str_utilts.h>

XAMP_STREAM_NAMESPACE_BEGIN

#ifdef XAMP_OS_WIN

namespace {
	constexpr auto kCDSpeedMultiplier = 176.4;
	constexpr double kCDBytesPerSecond = 176400;

	DWORD Drive2BassID(char driver_letter) {
		for (DWORD i = 0; i < 25; i++) {
			BASS_CD_INFO cdinfo{};
			if (LIB_BASS.CDLib->BASS_CD_GetInfo(i, &cdinfo)) {
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
		release();
	}

	void setAction(CDDeviceAction action) {
		BassIfFailedThrow(LIB_BASS.CDLib->BASS_CD_Door(driver_, static_cast<DWORD>(action)));
	}

	void setSpeed(uint32_t speed) {
		BassIfFailedThrow(LIB_BASS.CDLib->BASS_CD_SetSpeed(driver_, speed));
	}

	[[nodiscard]] uint32_t getSpeed() const {
		return static_cast<uint32_t>((LIB_BASS.CDLib->BASS_CD_GetSpeed(driver_) / kCDSpeedMultiplier));
	}

	[[nodiscard]] bool doorIsOpen() const {
		return LIB_BASS.CDLib->BASS_CD_DoorIsOpen(driver_);
	}

	std::string getISRC(uint32_t track) const {
		auto const* text = LIB_BASS.CDLib->BASS_CD_GetID(driver_, BASS_CDID_ISRC + track);
		if (!text) {
			return "";
		}
		return text;
	}

	[[nodiscard]] CDText getCDText() const {
		CDText cd_text;
		auto const * text = LIB_BASS.CDLib->BASS_CD_GetID(driver_, BASS_CDID_TEXT);
		if (!text) {
			return cd_text;
		}
		while (*text) {
			auto tag = String::split(text, "=");
			if (tag[0] == "TITLE") {
				cd_text.title = String::toStdWString(tag[1].data());
			}
			text += strlen(text) + 1;
		}
		return cd_text;
	}

	[[nodiscard]] std::vector<std::wstring> getTotalTracks() const {
		std::vector<std::wstring> tracks;
		const auto num_track = LIB_BASS.CDLib->BASS_CD_GetTracks(driver_);
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

	[[nodiscard]] CDDeviceInfo getCDDeviceInfo() const {
		BASS_CD_INFO info{};
		BassIfFailedThrow(LIB_BASS.CDLib->BASS_CD_GetInfo(driver_, &info));
		CDDeviceInfo device_info;
		device_info.can_lock = info.canlock;
		device_info.can_open = info.canopen;
		device_info.device_letter = std::to_wstring(info.letter);
		if (info.product != nullptr) {
			device_info.product = String::toStdWString(info.product);
		}		
		if (info.vendor != nullptr) {
			device_info.vendor = String::toStdWString(info.vendor);
		}		
		if (info.rev != nullptr) {
			device_info.rev = String::toStdWString(info.rev);
		}		
		device_info.cache_size = info.cache;
		device_info.max_speed = info.maxspeed;
		device_info.can_read_cdtext = info.cdtext;
		return device_info;
	}

	uint32_t getTrackLength(uint32_t track) const {
		return LIB_BASS.CDLib->BASS_CD_GetTrackLength(driver_, track);
	}

	void release() {
		LIB_BASS.CDLib->BASS_CD_Release(driver_);
	}

	void setMaxSpeed() {
		// -1 = optimal performace.
		BassIfFailedThrow(LIB_BASS.CDLib->BASS_CD_SetSpeed(driver_, -1));
	}

	double getDuration(uint32_t track) const {		
		return LIB_BASS.CDLib->BASS_CD_GetTrackLength(driver_, track) / kCDBytesPerSecond;
	}
private:
	char driver_letter_;
	DWORD driver_;
};

BassCDDevice::BassCDDevice(char driver_letter)
	: impl_(makeAlign<BassCDDeviceImpl>(driver_letter)) {
}

XAMP_PIMPL_IMPL(BassCDDevice)

void BassCDDevice::setAction(CDDeviceAction action) {
	impl_->setAction(action);
}

void BassCDDevice::setSpeed(uint32_t speed) {
	impl_->setSpeed(speed);
}

void BassCDDevice::setMaxSpeed() {
	impl_->setMaxSpeed();
}

uint32_t BassCDDevice::getSpeed() const {
	return impl_->getSpeed();
}

bool BassCDDevice::doorIsOpen() const {
	return impl_->doorIsOpen();
}

CDText BassCDDevice::getCDText() const {
	return impl_->getCDText();
}

std::vector<std::wstring> BassCDDevice::getTotalTracks() const {
	return impl_->getTotalTracks();
}

double BassCDDevice::getDuration(uint32_t track) const {
	return impl_->getDuration(track);
}

std::string BassCDDevice::getISRC(uint32_t track) const {
	return impl_->getISRC(track);
}

CDDeviceInfo BassCDDevice::getCDDeviceInfo() const {
	return impl_->getCDDeviceInfo();
}

void BassCDDevice::release() {
	impl_->release();
}

#endif

XAMP_STREAM_NAMESPACE_END
