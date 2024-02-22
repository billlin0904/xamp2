#include <base/vmmemlock.h>

#include <base/logger.h>
#include <base/str_utilts.h>
#include <base/stacktrace.h>
#include <base/exception.h>
#include <base/logger_impl.h>
#include <base/memory.h>
#include <base/platform.h>

XAMP_BASE_NAMESPACE_BEGIN

XAMP_DECLARE_LOG_NAME(VirtualMemory);

VmMemLock::VmMemLock() noexcept {
	logger_ = XampLoggerFactory.GetLogger(kVirtualMemoryLoggerName);
}

VmMemLock::~VmMemLock() noexcept {
	UnLock();
}

void VmMemLock::Lock(void* address, size_t size) {
	UnLock();	

	if (!VirtualMemoryLock(address, size)) { // try lock memory!
		throw PlatformException("VirtualLock return failure!");
	}

	// note: 強制配置實體記憶體page, 可以優化後面的相關操作. 等同於mmap API的MAP_POPULATE旗標.
	MemorySet(address, 0, size);
	address_ = address;
	size_ = size;

	XAMP_LOG_D(logger_,
		"VmMemLock lock address: 0x{:08x} size: {}.",
		reinterpret_cast<int64_t>(address_),
		String::FormatBytes(size_));
}

void VmMemLock::UnLock() noexcept {
	if (address_) {
		if (!VirtualMemoryUnLock(address_, size_)) {
			XAMP_LOG_D(logger_,
				"VirtualUnlock return failure! error:{} {}.",
				GetLastErrorMessage(), StackTrace{}.CaptureStack());
		}
		XAMP_LOG_D(logger_,
			"VmMemLock unlock address: 0x{:08x} size: {}.",
			reinterpret_cast<int64_t>(address_), String::FormatBytes(size_));
	}
	address_ = nullptr;
	size_ = 0;
}

VmMemLock& VmMemLock::operator=(VmMemLock&& other) noexcept {
	if (this != &other) {
		address_ = other.address_;
		size_ = other.size_;
		other.address_ = nullptr;
		other.size_ = 0;
	}
	return *this;
}

XAMP_BASE_NAMESPACE_END
