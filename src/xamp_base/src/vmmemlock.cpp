#include <base/logger.h>
#ifdef XAMP_OS_WIN
#include <base/windows_handle.h>
#else
#include <base/posix_handle.h>
#endif
#include <base/exception.h>
#include <base/memory.h>
#include <base/vmmemlock.h>

namespace xamp::base {

#ifdef XAMP_OS_WIN
static bool ExterndProcessWorkingSetSize(size_t size) noexcept {
    SIZE_T minimum = 0;
    SIZE_T maximum = 0;

    WinHandle current_process(::GetCurrentProcess());

    if (::GetProcessWorkingSetSize(current_process.get(), &minimum, &maximum)) {
        minimum += size;
        if (maximum < minimum + size) {
            maximum = minimum + size;
        }
        return ::SetProcessWorkingSetSize(current_process.get(), minimum, maximum);
    }
    return false;
}

static bool EnablePrivilege(std::string_view privilege, bool enable) noexcept {
	WinHandle current_process(::GetCurrentProcess());

	WinHandle token;
	HANDLE process_token;

	if (::OpenProcessToken(current_process.get(), TOKEN_ADJUST_PRIVILEGES, &process_token)) {
		token.reset(process_token);

		TOKEN_PRIVILEGES tp;
		tp.PrivilegeCount = 1;
		if (!::LookupPrivilegeValueA(nullptr, privilege.data(), &tp.Privileges[0].Luid)) {
			XAMP_LOG_DEBUG("LookupPrivilegeValueA return failure! error:{}", GetLastError());
			return false;
		}

		tp.Privileges->Attributes = enable ? SE_PRIVILEGE_ENABLED : 0;
		if (!::AdjustTokenPrivileges(token.get(), FALSE, &tp, sizeof(TOKEN_PRIVILEGES), nullptr, nullptr)) {
			XAMP_LOG_DEBUG("AdjustTokenPrivileges return failure! error:{}", GetLastError());
			return false;
		}

		return true;
	}

	XAMP_LOG_DEBUG("OpenProcessToken return failure! error:{}", GetLastError());
	return false;
}

VmMemLock::VmMemLock() noexcept
	: address_(nullptr)
	, size_(0) {	
}

VmMemLock::~VmMemLock() noexcept {
	UnLock();
}

bool VmMemLock::EnableLockMemPrivilege(bool enable) noexcept {
	//return EnablePrivilege("SeLockMemoryPrivilege", enable);
	return true;
}

void VmMemLock::Lock(void* address, size_t size) {
	//UnLock();

	//if (!ExterndProcessWorkingSetSize(size)) {
	//	throw PlatformSpecException("ExterndProcessWorkingSetSize return failure! error:{}", ::GetLastError());
	//}

	//if (!::VirtualLock(address, size)) {
	//	throw PlatformSpecException("VirtualLock return failure! error:{}", ::GetLastError());
	//}

	//address_ = address;
	//size_ = size;
}

void VmMemLock::UnLock() noexcept {
	if (address_) {
		if (!::VirtualUnlock(address_, size_)) {
			XAMP_LOG_DEBUG("VirtualUnlock return failure! error:{}", ::GetLastError());
		}
	}
	address_ = nullptr;
	size_ = 0;
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
		throw PlatformSpecException("mlock return failure! error:{}", errno);
    }

    address_ = address;
    size_ = size;
}

void VmMemLock::UnLock() noexcept {
    if (address_) {
        if (::munlock(address_, size_)) {
            XAMP_LOG_DEBUG("munlock return failure! error:{}", errno);
        }
    }
    address_ = nullptr;
    size_ = 0;
}

#endif

}
