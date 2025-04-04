#include <output_device/win32/mmcss.h>

#ifdef XAMP_OS_WIN

#include <base/dll.h>
#include <base/base.h>
#include <base/logger.h>
#include <base/logger_impl.h>
#include <base/exception.h>

#include <avrt.h>

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_BEGIN

class Mmcss::MmcssImpl {
public:
	MmcssImpl() 
		: avrt_task_index_(0)
		, avrt_handle_(nullptr) {
	}

	void BoostPriority(std::wstring_view task_name, MmcssThreadPriority priority) noexcept {
		RevertPriority();

		avrt_handle_ = ::AvSetMmThreadCharacteristicsW(task_name.data(), &avrt_task_index_);
		if (avrt_handle_ != nullptr) {
			if (!::AvSetMmThreadPriority(avrt_handle_, static_cast<AVRT_PRIORITY>(priority))) {
				XAMP_LOG_ERROR("AvSetMmThreadPriority return failure! Error:{}",
					GetLastErrorMessage());
			}
			return;
		}
		
		XAMP_LOG_ERROR("AvSetMmThreadCharacteristicsW return failure! Error:{}",
			GetLastErrorMessage());
	}

	void RevertPriority() noexcept {
		if (!avrt_handle_) {
			return;
		}
		
		if (!::AvRevertMmThreadCharacteristics(avrt_handle_)) {
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

void Mmcss::BoostPriority(std::wstring_view task_name, MmcssThreadPriority priority) noexcept {
	impl_->BoostPriority(task_name, priority);
}

void Mmcss::RevertPriority() noexcept {
	impl_->RevertPriority();
}

XAMP_OUTPUT_DEVICE_WIN32_NAMESPACE_END

#endif

