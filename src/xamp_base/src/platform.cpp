#include <base/platform.h>

#include <exception>

#include <base/assert.h>
#include <base/waitabletimer.h>

#include <cerrno>
#include <sstream>
#include <thread>

XAMP_BASE_NAMESPACE_BEGIN

namespace {
    uint64_t toMilliseconds(const timespec* ts) {
        return static_cast<uint64_t>(ts->tv_sec) * 1000 + ts->tv_nsec / 1000000;
    }
}

int32_t atomicWait(std::atomic<uint32_t>& to_wait_on, uint32_t expected, const timespec* to) {
    if (to == nullptr) {
        if (!atomicWait(to_wait_on, expected, kInfinity)) {
            errno = EINTR;
            return -1;
        }
        return 0;
    }

    XAMP_EXPECTS(to->tv_nsec >= 0);

    if (to->tv_nsec >= 1000000000) {
        errno = EINVAL;
        return -1;
    }

    if (to->tv_sec >= 2147) {
        // atomicWait 的 millisecond overload 以 uint32_t 表示 timeout。
        // timespec 太大時直接轉成毫秒可能 overflow；這裡改等一個足夠大的安全值
        // (2147000000ms，約 24.8 天)，若真的等完仍未醒來，就回報成功，
        // 交給上層 condition-variable predicate 當成 spurious wake 再檢查一次。
        atomicWait(to_wait_on, expected, 2147000000);
        return 0;
    }

    if (!atomicWait(to_wait_on, expected, static_cast<uint32_t>(toMilliseconds(to)))) {
        errno = ETIMEDOUT;
        return -1;
    }
    return 0;
}

std::string getCurrentThreadId() {
    std::ostringstream ostr;
    ostr << std::this_thread::get_id();
    return ostr.str();
}

uint64_t getSystemEntropy() {
    const auto r0{ genRandomSeed() };
    const auto r1{ genRandomSeed() };
    return (r1 << 32) | (r0 & UINT64_C(0xffffffff));
}

XAMP_BASE_NAMESPACE_END
