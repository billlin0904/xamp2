#ifdef _WIN32

#include <base/dll.h>
#include <base/windows_handle.h>
#include <avrt.h>

#include <base/logger.h>
#include <output_device/win32/mmcss_types.h>

namespace xamp::output_device::win32 {

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
		avrt_handle_ = AvrtLib::Instance().AvSetMmThreadCharacteristicsW(L"Pro Audio", &avrt_task_index_);		
		if (avrt_handle_ != nullptr) {
			AvrtLib::Instance().AvSetMmThreadPriority(avrt_handle_, AVRT_PRIORITY_HIGH);
		} else {
			auto last_error = ::GetLastError();
			XAMP_LOG_ERROR("AvSetMmThreadCharacteristicsW return failure! {}",
				Exception::GetPlatformErrorMessage(last_error));
		}
	}

	void RevertPriority() {
		if (!avrt_handle_) {
			return;
		}
		DWORD last_error = 0;
		if (!AvrtLib::Instance().AvRevertMmThreadCharacteristics(avrt_handle_)) {
			last_error = ::GetLastError();
			XAMP_LOG_ERROR("AvSetMmThreadCharacteristicsW return failure! {}",
				Exception::GetPlatformErrorMessage(last_error));
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
#endif

