#include <base/base.h>

#ifdef XAMP_OS_WIN

#include <base/dll.h>
#include <base/windows_handle.h>
#include <avrt.h>

#include <base/logger.h>
#include <output_device/win32/mmcss.h>

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
		: module_(LoadDll("Avrt.dll"))
		, AvRevertMmThreadCharacteristics(module_, "AvRevertMmThreadCharacteristics")
		, AvSetMmThreadPriority(module_, "AvSetMmThreadPriority")
		, AvSetMmThreadCharacteristicsW(module_, "AvSetMmThreadCharacteristicsW") {
	}

	ModuleHandle module_;

public:
	XAMP_DLL_C_API(AvRevertMmThreadCharacteristics)
	XAMP_DLL_C_API(AvSetMmThreadPriority)
	XAMP_DLL_C_API(AvSetMmThreadCharacteristicsW)
};

class Mmcss::MmcssImpl {
public:
	MmcssImpl() 
		: avrt_task_index_(0)
		, avrt_handle_(nullptr) {
	}

	void BoostPriority(std::wstring_view task_name) {
		RevertPriority();

		avrt_handle_ = AvrtLib::Instance().AvSetMmThreadCharacteristicsW(task_name.data(), &avrt_task_index_);
		if (avrt_handle_ != nullptr) {
			AvrtLib::Instance().AvSetMmThreadPriority(avrt_handle_, AVRT_PRIORITY_HIGH);
			return;
		}
		
		auto last_error = ::GetLastError();
		XAMP_LOG_ERROR("AvSetMmThreadCharacteristicsW return failure! Error:{} {}",
			last_error,
			Exception::GetPlatformErrorMessage(last_error));
	}

	void RevertPriority() {
		if (!avrt_handle_) {
			return;
		}
		
		if (!AvrtLib::Instance().AvRevertMmThreadCharacteristics(avrt_handle_)) {
			auto last_error = ::GetLastError();
			XAMP_LOG_ERROR("AvSetMmThreadCharacteristicsW return failure! Error:{} {}",
				last_error,
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

void Mmcss::LoadAvrtLib() {
	AvrtLib::Instance();
}

void Mmcss::BoostPriority(std::wstring_view task_name) {
	impl_->BoostPriority(task_name);
}

void Mmcss::RevertPriority() {
	impl_->RevertPriority();
}

}
#endif

