#include <base/str_utilts.h>
#include <stream/basslib.h>
#include <stream/basscddevice.h>

namespace xamp::stream {

class BassCDDevice::BassCDDeviceImpl {
public:
	explicit BassCDDeviceImpl(uint32_t driver)
		: driver_(driver) {
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

	uint32_t GetSpeed() const {
		return BASS.CDLib->BASS_CD_GetSpeed(driver_);
	}

	bool DoorIsOpen() const {
		return BASS.CDLib->BASS_CD_DoorIsOpen(driver_);
	}

	CDText GetCDText() const {
		CDText cd_text;
		auto const * text = BASS.CDLib->BASS_CD_GetID(driver_, BASS_CDID_TEXT);
		if (!text) {
			return cd_text;
		}
		while (*text) {
			auto tag = String::Split(text, "=");
			text += strlen(text) + 1;
		}
		return cd_text;
	}

	uint32_t GetTotalTracks() const {
		return BASS.CDLib->BASS_CD_GetTracks(driver_);
	}

	CDDeviceInfo GetCDDeviceInfo() const {
		BASS_CD_INFO info{};
		BassIfFailedThrow(BASS.CDLib->BASS_CD_GetInfo(driver_, &info));
		CDDeviceInfo device_info;
		device_info.can_lock = info.canlock;
		device_info.can_open = info.canopen;
		device_info.device_letter = std::to_wstring(info.letter);
		device_info.product = String::ToStdWString(info.product);
		device_info.vendor = String::ToStdWString(info.vendor);
		device_info.rev = String::ToStdWString(info.rev);
		device_info.can_cdtext = info.cdtext;
		return device_info;
	}

	void Release() {
		BASS.CDLib->BASS_CD_Release(driver_);
	}
private:
	DWORD driver_;
};

BassCDDevice::BassCDDevice(uint32_t driver)
	: impl_(MakeAlign<BassCDDeviceImpl>(driver)) {
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

uint32_t BassCDDevice::GetTotalTracks() const {
	return impl_->GetTotalTracks();
}

CDDeviceInfo BassCDDevice::GetCDDeviceInfo() const {
	return impl_->GetCDDeviceInfo();
}

void BassCDDevice::Release() {
	impl_->Release();
}

}

