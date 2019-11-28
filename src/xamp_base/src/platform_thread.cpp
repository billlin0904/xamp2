#include <base/dll.h>
#include <base/str_utilts.h>
#include <base/platform_thread.h>

#ifdef _WIN32
#include <base/windows_handle.h>
#endif

namespace xamp::base {

#ifdef _WIN32
#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
    DWORD dwType; // Must be 0x1000.
    LPCSTR szName; // Pointer to name (in user addr space).
    DWORD dwThreadID; // Thread ID (-1=caller thread).
    DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

void SetThreadNameById(DWORD dwThreadID, const char* threadName) {
	constexpr DWORD MS_VC_EXCEPTION = 0x406D1388;

    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = threadName;
    info.dwThreadID = dwThreadID;
    info.dwFlags = 0;

    __try {
        ::RaiseException(MS_VC_EXCEPTION, 
            0,
            sizeof(info) / sizeof(ULONG_PTR), 
            reinterpret_cast<ULONG_PTR*>(&info));
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
}
#endif

void PlatformThread::SetThreadName(const std::string& name) {
#ifdef _WIN32
	WinHandle thread(::GetCurrentThread());

	try {
		// At Windows 10 1607 Supported.
		// The SetThreadDescription API works even if no debugger is attached.
		DllFunction<HRESULT(HANDLE hThread, PCWSTR lpThreadDescription)>
			SetThreadDescription(LoadDll("Kernel32.dll"), "SetThreadDescription");

		SetThreadDescription(thread.get(), ToStdWString(name).c_str());
	}
	catch (...) {
	}	

	// The debugger needs to be around to catch the name in the exception.  If
	// there isn't a debugger, we are just needlessly throwing an exception.
	if (!::IsDebuggerPresent()) {
		return;
	}

	SetThreadNameById(::GetCurrentThreadId(), name.c_str());
#else
	// Mac OS X does not expose the length limit of the name, so
	// hardcode it.
	const int kMaxNameLength = 63;
	std::string shortened_name = name.substr(0, kMaxNameLength);
	pthread_setname_np(shortened_name.c_str());
#endif
}
	
}
