#include <base/vmmemlock.h>

#include <base/logger.h>
#include <base/str_utilts.h>
#include <base/stacktrace.h>
#include <base/exception.h>
#include <base/memory.h>
#include <base/platform.h>

#ifdef XAMP_OS_WIN
#include <base/platfrom_handle.h>
#endif

XAMP_BASE_NAMESPACE_BEGIN

XAMP_DECLARE_LOG_NAME(VirtualMemory);

VmMemLock::VmMemLock() {
	logger_ = XAMP_LOG_CREATE_LOGGER(VirtualMemory);
}

VmMemLock::~VmMemLock() {
	UnLock();
}

void VmMemLock::Lock(void* address, size_t size) {
	UnLock();	

	if (address == nullptr || size == 0) {
		return;
	}

	// note: 強制配置實體記憶體page, 可以優化後面的相關操作. 等同於mmap API的MAP_POPULATE旗標.
	MemorySet(address, 0, size);

	if (!VirtualMemoryLock(address, size)) { // try lock memory!
		XAMP_LOG_E(logger_, "VirtualLock return failure! {}", GetLastErrorMessage());
		return;
	}

	address_ = address;
	size_ = size;

	XAMP_LOG_T(logger_,
		"VmMemLock lock address: 0x{:08x} size: {}.",
		reinterpret_cast<int64_t>(address_),
		String::FormatBytes(size_));
}

void VmMemLock::UnLock() {
	if (address_) {
		if (!VirtualMemoryUnLock(address_, size_)) {
#ifdef XAMP_OS_WIN
			const auto last_error = ::GetLastError();
			if (last_error != ERROR_NOT_LOCKED) {
				XAMP_LOG_E(logger_,
					"VirtualUnlock return failure! error:{} {}.",
					GetPlatformErrorMessage(static_cast<int32_t>(last_error)),
					StackTrace{}.CaptureStack());
			}
#endif
		}
		XAMP_LOG_T(logger_,
			"VmMemLock unlock address: 0x{:08x} size: {}.",
			reinterpret_cast<int64_t>(address_),
			String::FormatBytes(size_));
	}
	address_ = nullptr;
	size_ = 0;
}

VmMemLock& VmMemLock::operator=(VmMemLock&& other) {
	if (this != &other) {
		// Move assignment replaces the lock owned by *this, so release the
		// current pages before taking ownership from other.
		UnLock();
		address_ = other.address_;
		size_ = other.size_;
		other.address_ = nullptr;
		other.size_ = 0;
	}
	return *this;
}

XAMP_BASE_NAMESPACE_END
