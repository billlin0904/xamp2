// This is an open source non-commercial project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include <base/waitabletimer.h>

namespace xamp::base {

WaitableTimer::WaitableTimer() noexcept
	: timeout_(0)
    , tp_(std::chrono::steady_clock::now()) {
}

}
