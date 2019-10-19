#if 0
#ifdef _WIN32
#include <base/windows_handle.h>
#endif

#include <base/threadpool.h>

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
	static const DWORD MS_VC_EXCEPTION = 0x406D1388;

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

namespace xamp::base {

void SetCurrentThreadName(const std::string &name) {
#ifdef _WIN32
    auto thread_id = ::GetThreadId(::GetCurrentThread());
    SetThreadNameById(thread_id, name.c_str());
#endif
}

void SetCurrentThreadName(int32_t index) {
	std::ostringstream ostr;
	ostr << "Work Thread(" << index << ")";
	const auto thread_name = ostr.str();

	SetCurrentThreadName(thread_name);
}

ThreadPool::ThreadPool()
    : scheduler_() {
}

}
#endif
