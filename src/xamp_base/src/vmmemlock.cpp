#include <base/logger.h>
#include <base/windows_handle.h>
#include <base/memory.h>
#include <base/vmmemlock.h>

namespace xamp::base {

#ifdef _WIN32
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

static bool EnablePrivilege(const std::string& privilege, bool enable) noexcept {
	WinHandle current_process(::GetCurrentProcess());

	WinHandle token;
	HANDLE process_token;

	if (::OpenProcessToken(current_process.get(), TOKEN_ADJUST_PRIVILEGES, &process_token)) {
		token.reset(process_token);

		TOKEN_PRIVILEGES tp;
		tp.PrivilegeCount = 1;
		if (!::LookupPrivilegeValueA(nullptr, privilege.c_str(), &tp.Privileges[0].Luid)) {
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

bool VmMemLock::EnableVmMemPrivilege(bool enable) noexcept {
	return EnablePrivilege("SeLockMemoryPrivilege", enable);
}

void VmMemLock::Lock(void* address, size_t size) noexcept {
	UnLock();

	if (!ExterndProcessWorkingSetSize(size)) {
		XAMP_LOG_DEBUG("ExterndProcessWorkingSetSize return failure! error:{}", GetLastError());
		return;
	}

	if (!::VirtualLock(address_, size_)) {
		XAMP_LOG_DEBUG("VirtualLock return failure! error:{}", GetLastError());
	}

	address_ = address;
	size_ = size;
}

void VmMemLock::UnLock() noexcept {
	if (address_) {
		if (!::VirtualUnlock(address_, size_)) {
			XAMP_LOG_DEBUG("VirtualUnlock return failure! error:{}", GetLastError());
		}
	}
	address_ = nullptr;
	size_ = 0;
}
#endif

}
