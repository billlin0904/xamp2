#include <base/logger.h>
#ifdef XAMP_OS_WIN
#include <base/windows_handle.h>
#else
#include <sys/mman.h>
#endif
#include <base/exception.h>
#include <base/vmmemlock.h>

namespace xamp::base {
#ifdef XAMP_OS_WIN
static bool ExterndProcessWorkingSetSize(size_t size) noexcept {
    SIZE_T minimum = 0;
    SIZE_T maximum = 0;

    const WinHandle current_process(::GetCurrentProcess());

    if (::GetProcessWorkingSetSize(current_process.get(), &minimum, &maximum)) {
        minimum += size;
        if (maximum < minimum + size) {
            maximum = minimum + size;
        }
        return ::SetProcessWorkingSetSize(current_process.get(), minimum, maximum);
    }
    return false;
}

VmMemLock::VmMemLock() noexcept
	: address_(nullptr)
	, size_(0) {	
}

VmMemLock::~VmMemLock() noexcept {
	UnLock();
}

void VmMemLock::Lock(void* address, size_t size) {
#if 1
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
#endif
}

void VmMemLock::UnLock() noexcept {
#if 1
	if (address_) {
		if (!::VirtualUnlock(address_, size_)) {
			XAMP_LOG_DEBUG("VirtualUnlock return failure! error:{}.", ::GetLastError());
		}
		XAMP_LOG_DEBUG("VmMemLock unlock address: 0x{:08x} size: {}.", int64_t(address_), size_);
	}
	address_ = nullptr;
	size_ = 0;
#endif
}
#else
VmMemLock::VmMemLock() noexcept
    : address_(nullptr)
    , size_(0) {
}

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

}
