#ifdef _WIN32
#include <base/dll.h>
#include <base/windows_handle.h>
#include <avrt.h>
#endif

#include <output_device/win32/mmcss_types.h>

namespace xamp::output_device::win32 {

const std::wstring_view MMCSS_PROFILE_AUDIO(L"Audio");
const std::wstring_view MMCSS_PROFILE_CAPTURE(L"Capture");
const std::wstring_view MMCSS_MODE_DISTRIBUTION(L"Distribution");
const std::wstring_view MMCSS_PROFILE_GAME(L"Games");
const std::wstring_view MMCSS_PROFILE_PLAYBACK(L"Playback");
const std::wstring_view MMCSS_PROFILE_PRO_AUDIO(L"Pro Audio");
const std::wstring_view MMCSS_PROFILE_WINDOWS_MANAGER(L"Window Manager");

class AvrtLib {
public:
	static AvrtLib& Instance() {
		static AvrtLib instance;
		return instance;
	}

	XAMP_DISABLE_COPY(AvrtLib)

private:
	AvrtLib()
		: module_(LoadDll("avrt.dll"))
		, AvRevertMmThreadCharacteristics(module_, "AvRevertMmThreadCharacteristics")
		, AvSetMmThreadPriority(module_, "AvSetMmThreadPriority")
		, AvSetMmThreadCharacteristicsW(module_, "AvSetMmThreadCharacteristicsW") {
	}

	ModuleHandle module_;

public:
	XAMP_DEFINE_DLL_API(AvRevertMmThreadCharacteristics) AvRevertMmThreadCharacteristics;
	XAMP_DEFINE_DLL_API(AvSetMmThreadPriority) AvSetMmThreadPriority;
	XAMP_DEFINE_DLL_API(AvSetMmThreadCharacteristicsW) AvSetMmThreadCharacteristicsW;
};

class Mmcss::MmcssImpl {
public:
	MmcssImpl() 
		: avrt_task_index_(0)
		, avrt_handle_(nullptr) {
	}

	void BoostPriority() {
		RevertPriority();
		WinHandle thread(::GetCurrentThread());
		::SetThreadPriority(thread.get(), THREAD_PRIORITY_HIGHEST);
		avrt_handle_ = AvrtLib::Instance().AvSetMmThreadCharacteristicsW(L"Pro Audio", &avrt_task_index_);
		auto last_error = ::GetLastError();
		if (avrt_handle_ != nullptr && last_error == ERROR_SUCCESS) {
			AvrtLib::Instance().AvSetMmThreadPriority(avrt_handle_, AVRT_PRIORITY_HIGH);
		}
	}

	void RevertPriority() {
		if (!avrt_handle_) {
			return;
		}
		DWORD last_error = 0;
		if (!AvrtLib::Instance().AvRevertMmThreadCharacteristics(avrt_handle_)) {
			last_error = ::GetLastError();
		}
		avrt_handle_ = nullptr;
		avrt_task_index_ = 0;
	}
private:
	DWORD avrt_task_index_;
	HANDLE avrt_handle_;
};

Mmcss::Mmcss()
	: impl_(MakeAlign<MmcssImpl>()) {
}

Mmcss::~Mmcss() {
}

void Mmcss::BoostPriority() {
	impl_->BoostPriority();
}

void Mmcss::RevertPriority() {
	impl_->RevertPriority();
}

}
