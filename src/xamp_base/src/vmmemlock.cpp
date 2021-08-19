#include <base/logger.h>
#ifdef XAMP_OS_WIN
#include <base/windows_handle.h>
#else
#include <sys/mman.h>
#endif
#include <base/str_utilts.h>
#include <base/stacktrace.h>
#include <base/exception.h>
#include <base/platform.h>
#include <base/vmmemlock.h>

namespace xamp::base {

VmMemLock::VmMemLock() noexcept = default;

VmMemLock::~VmMemLock() noexcept {
	UnLock();
}
	
#ifdef XAMP_OS_WIN
void VmMemLock::Lock(void* address, size_t size) {
	UnLock();	

	if (!::VirtualLock(address, size)) { // try lock memory!
		if (!ExtendProcessWorkingSetSize(size)) {
			throw PlatformSpecException("ExtendProcessWorkingSetSize return failure!");
		}
		if (!::VirtualLock(address, size)) {
			throw PlatformSpecException("VirtualLock return failure!");
		}		
	}

	address_ = address;
	size_ = size;
#ifdef XAMP_ENABLE_THREAD_POOL_DEBUG
	XAMP_LOG_D(Logger::GetInstance().GetLogger(kVirtualMemoryLoggerName),
		"VmMemLock lock address: 0x{:08x} size: {}.",
		reinterpret_cast<int64_t>(address_),
		String::FormatBytes(size_));
#endif
}

void VmMemLock::UnLock() noexcept {
	if (address_) {
		if (!::VirtualUnlock(address_, size_)) {
#ifdef XAMP_ENABLE_THREAD_POOL_DEBUG
			XAMP_LOG_D(Logger::GetInstance().GetLogger(kVirtualMemoryLoggerName),
				"VirtualUnlock return failure! error:{} {}.",
				GetLastErrorMessage(), StackTrace{}.CaptureStack());
#endif
		}
#ifdef XAMP_ENABLE_THREAD_POOL_DEBUG
		XAMP_LOG_D(Logger::GetInstance().GetLogger(kVirtualMemoryLoggerName), 
			"VmMemLock unlock address: 0x{:08x} size: {}.",
			reinterpret_cast<int64_t>(address_), String::FormatBytes(size_));
#endif
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
