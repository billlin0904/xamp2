#include <base/logger.h>
#ifdef XAMP_OS_WIN
#include <base/windows_handle.h>
#else
#include <sys/mman.h>
#endif
#include <base/str_utilts.h>
#include <base/stacktrace.h>
#include <base/exception.h>
#include <base/platform_thread.h>
#include <base/vmmemlock.h>

namespace xamp::base {

#ifdef XAMP_OS_WIN
class VmMemLock::VmMemLockImpl {
public:
	VmMemLockImpl() {		
	}
	
	~VmMemLockImpl() noexcept {
		UnLock();
	}
	
	void Lock(void* address, size_t size) {
		UnLock();

		if (!ExtendProcessWorkingSetSize(size)) {
			throw PlatformSpecException("ExtendProcessWorkingSetSize return failure!");
		}

		if (!::VirtualLock(address, size)) {
			throw PlatformSpecException("VirtualLock return failure!");
		}

		address_ = address;
		size_ = size;

		XAMP_LOG_DEBUG("VmMemLock lock address: 0x{:08x} size: {}.",
			reinterpret_cast<int64_t>(address_), String::FormatBytes(size_));
	}
	
	void UnLock() noexcept {
		if (address_) {
			if (!::VirtualUnlock(address_, size_)) {
				XAMP_LOG_DEBUG("VirtualUnlock return failure! error:{} {}.",
					::GetLastError(), StackTrace{}.CaptureStack());
			}
			XAMP_LOG_DEBUG("VmMemLock unlock address: 0x{:08x} size: {}.",
				reinterpret_cast<int64_t>(address_), String::FormatBytes(size_));
		}
		address_ = nullptr;
		size_ = 0;
	}
	void* address_{nullptr};
	size_t size_{0};
};
#else
	class VmMemLock::VmMemLockImpl {
public:
	VmMemLockImpl() {		
	}
	
	~VmMemLockImpl() noexcept {
		UnLock();
	}
	
	void Lock(void* address, size_t size) {
		UnLock();

		if (::mlock(address, size)) {
			throw PlatformSpecException("ExtendProcessWorkingSetSize return failure!");
		}

		address_ = address;
		size_ = size;
	}
	
	void UnLock() noexcept {
		if (address_) {
			if (::munlock(address_, size_)) {
				XAMP_LOG_DEBUG("munlock return failure! error:{}.", errno);
			}
		}
		address_ = nullptr;
		size_ = 0;
	}
	void* address_{nullptr};
	size_t size_{0};
};
#endif

XAMP_PIMPL_IMPL(VmMemLock)
	
VmMemLock::VmMemLock() noexcept
	: impl_(MakeAlign<VmMemLockImpl>()) {
}

VmMemLock::VmMemLock(void* address, size_t size)
	: VmMemLock() {
	Lock(address, size);
}
	
void VmMemLock::Lock(void* address, size_t size) {
	impl_->Lock(address, size);
}

void VmMemLock::UnLock() noexcept {
	impl_->UnLock();
}

}
