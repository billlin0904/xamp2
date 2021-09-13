#include <base/base.h>

#ifdef XAMP_OS_WIN

#include <base/dll.h>
#include <base/windows_handle.h>
#include <base/singleton.h>
#include <avrt.h>

#include <base/logger.h>
#include <output_device/win32/mmcss.h>

namespace xamp::output_device::win32 {

class AvrtLib {
public:
	friend class Singleton<AvrtLib>;

	XAMP_DISABLE_COPY(AvrtLib)

private:
	AvrtLib()
		: module_(LoadModule("Avrt.dll"))
		, AvRevertMmThreadCharacteristics(module_, "AvRevertMmThreadCharacteristics")
		, AvSetMmThreadPriority(module_, "AvSetMmThreadPriority")
		, AvSetMmThreadCharacteristicsW(module_, "AvSetMmThreadCharacteristicsW") {
	}

	ModuleHandle module_;

public:
	XAMP_DECLARE_DLL(AvRevertMmThreadCharacteristics) AvRevertMmThreadCharacteristics;
	XAMP_DECLARE_DLL(AvSetMmThreadPriority) AvSetMmThreadPriority;
	XAMP_DECLARE_DLL(AvSetMmThreadCharacteristicsW) AvSetMmThreadCharacteristicsW;
};

class Mmcss::MmcssImpl {
public:
	MmcssImpl() 
		: avrt_task_index_(0)
		, avrt_handle_(nullptr) {
	}

	void BoostPriority(std::wstring_view task_name) noexcept {
		RevertPriority();

		avrt_handle_ = Singleton<AvrtLib>::GetInstance().AvSetMmThreadCharacteristicsW(task_name.data(), &avrt_task_index_);
		if (avrt_handle_ != nullptr) {
			Singleton<AvrtLib>::GetInstance().AvSetMmThreadPriority(avrt_handle_, AVRT_PRIORITY_HIGH);
			return;
		}
		
		XAMP_LOG_ERROR("AvSetMmThreadCharacteristicsW return failure! Error:{}",
			GetLastErrorMessage());
	}

	void RevertPriority() noexcept {
		if (!avrt_handle_) {
			return;
		}
		
		if (!Singleton<AvrtLib>::GetInstance().AvRevertMmThreadCharacteristics(avrt_handle_)) {
			XAMP_LOG_ERROR("AvSetMmThreadCharacteristicsW return failure! Error:{}",
				GetLastErrorMessage());
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

XAMP_PIMPL_IMPL(Mmcss)

void Mmcss::LoadAvrtLib() {
	(void)Singleton<AvrtLib>::GetInstance();
}

void Mmcss::BoostPriority(std::wstring_view task_name) noexcept {
	impl_->BoostPriority(task_name);
}

void Mmcss::RevertPriority() noexcept {
	impl_->RevertPriority();
}

}
#endif

