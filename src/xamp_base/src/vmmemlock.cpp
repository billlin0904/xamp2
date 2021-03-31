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

VmMemLock::VmMemLock() noexcept {
}

VmMemLock::~VmMemLock() noexcept {
	UnLock();
}
	
#ifdef XAMP_OS_WIN
void VmMemLock::Lock(void* address, size_t size) {
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

void VmMemLock::UnLock() noexcept {
	if (address_) {
		if (!::VirtualUnlock(address_, size_)) {
			XAMP_LOG_DEBUG("VirtualUnlock return failure! error:{} {}.",
				GetLastErrorMessage(), StackTrace{}.CaptureStack());
		}
		XAMP_LOG_DEBUG("VmMemLock unlock address: 0x{:08x} size: {}.",
			reinterpret_cast<int64_t>(address_), String::FormatBytes(size_));
	}
	address_ = nullptr;
	size_ = 0;
}
#else
void VmMemLock::Lock(void* address, size_t size) {
	UnLock();

	if (::mlock(address, size)) {
		throw PlatformSpecException("ExtendProcessWorkingSetSize return failure!");
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
VmMemLock& VmMemLock::operator=(VmMemLock&& other) noexcept {
	if (this != &other) {
		address_ = other.address_;
		size_ = other.size_;
		other.address_ = nullptr;
		other.size_ = 0;
	}
	return *this;
}

}
