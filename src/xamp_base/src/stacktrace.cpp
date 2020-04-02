#include <cstdio>
#include <csignal>
#include <vector>

#ifdef _WIN32
#else
#include <execinfo.h>
#endif

#include <base/logger.h>
#include <base/stacktrace.h>

namespace xamp::base {

#ifdef _WIN32
#else
static void PrintStackTrace(uint32_t max_frame_size) {
    std::vector<void*> addrlist(max_frame_size + 1);
    auto addrlen = backtrace(addrlist.data(), static_cast<int32_t>(addrlist.size()));
    if (addrlen == 0) {
        return;
    }

    auto symbollist = backtrace_symbols(addrlist.data(), addrlen);
    for (auto i = 4; i < addrlen; i++) {
        XAMP_LOG_DEBUG("{}", symbollist[i]);
    }
}
#endif

StackTrace::StackTrace() {
    signal(SIGABRT, AbortHandler);
    signal(SIGSEGV, AbortHandler);
    signal(SIGILL,  AbortHandler);
    signal(SIGFPE,  AbortHandler);
}

void StackTrace::AbortHandler(int32_t signum) {
    const char* name = nullptr;

    switch (signum) {
    case SIGABRT: name = "SIGABRT";  break;
    case SIGSEGV: name = "SIGSEGV";  break;
    case SIGBUS:  name = "SIGBUS";   break;
    case SIGILL:  name = "SIGILL";   break;
    case SIGFPE:  name = "SIGFPE";   break;
    }

    XAMP_LOG_DEBUG("Caught signal {} {}", signum, !name ? "" : name);

    constexpr uint32_t MaxStackFrameSize = 32;
    PrintStackTrace(MaxStackFrameSize);
}

}
