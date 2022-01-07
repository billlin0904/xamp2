#include <base/assert.h>

#ifdef XAMP_OS_WIN
#include <base/windows_handle.h>
#include <debugapi.h>
#else
#include <stdio.h>
#endif

namespace xamp::base {

Asserter::Asserter(bool hold)
	: hold_(hold) {
}

bool Asserter::Handle(const char* file, int line, const char* expr) const {
    if (hold_) {
        return true;
    }

    switch (AskUser(file, line, expr)) {
    case AssertHandleTypes::ASSERT_GIVE_UP:
    	abort();
    case AssertHandleTypes::ASSERT_DEBUG:
    	return false;
    case AssertHandleTypes::ASSERT_IGNORE:
    default:
    	return true;
    }
}

AssertHandleTypes Asserter::AskUser(const char* file, int line, const char* expr) const {
    return AssertHandleTypes::ASSERT_DEBUG;
}

Asserter Asserter::MakeAsserter(bool flag) {
    return { flag };
}

void Asserter::DebugTrap() {
#ifdef XAMP_OS_WIN
	::DebugBreak();
#else
	__builtin_debugtrap();
#endif
}


}
