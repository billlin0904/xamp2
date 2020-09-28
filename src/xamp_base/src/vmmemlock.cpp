#include <base/logger.h>
#ifdef XAMP_OS_WIN
#include <base/windows_handle.h>
#else
#include <sys/mman.h>
#endif
#include <base/stacktrace.h>
#include <base/exception.h>
#include <base/platform_thread.h>
#include <base/vmmemlock.h>

namespace xamp::base {
#ifdef XAMP_OS_WIN
VmMemLock::~VmMemLock() noexcept {
	UnLock();
}

void VmMemLock::Lock(void* address, size_t size) {
	UnLock();

	if (!ExterndProcessWorkingSetSize(size)) {
		throw PlatformSpecException("ExterndProcessWorkingSetSize return failure!");
	}

	if (!::VirtualLock(address, size)) {
		throw PlatformSpecException("VirtualLock return failure!");
	}

	address_ = address;
	size_ = size;

	XAMP_LOG_DEBUG("VmMemLock lock address: 0x{:08x} size: {}.", int64_t(address_), size_);
}

void VmMemLock::UnLock() noexcept {
	if (address_) {
		if (!::VirtualUnlock(address_, size_)) {
			XAMP_LOG_DEBUG("VirtualUnlock return failure! error:{} {}.", ::GetLastError(), StackTrace{}.CaptureStack());
		}
		XAMP_LOG_DEBUG("VmMemLock unlock address: 0x{:08x} size: {}.", int64_t(address_), size_);
	}
	address_ = nullptr;
	size_ = 0;
}
#else
VmMemLock::~VmMemLock() noexcept {
    UnLock();
}

void VmMemLock::Lock(void* address, size_t size) {
    UnLock();

    if (::mlock(address, size)) {
		throw PlatformSpecException("mlock return failure! error:{}.", errno);
    }

    address_ = address;
    size_ = size;
}

void VmMemLock::UnLock() noexcept {
    if (address_) {
        if (::munlock(address_, size_)) {
            XAMP_LOG_DEBUG("munlock return failure! error:{}.", errno);
        }
    }
    address_ = nullptr;
    size_ = 0;
}
#endif

VmMemLock::VmMemLock() noexcept
	: address_(nullptr)
	, size_(0) {
}

VmMemLock::VmMemLock(void* address, size_t size)
	: VmMemLock() {
	Lock(address, size);
}

}
