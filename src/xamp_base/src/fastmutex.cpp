#include <base/fastmutex.h>

namespace xamp::base {

#ifdef XAMP_OS_WIN
SRWMutex::SRWMutex() noexcept {
	::InitializeSRWLock(&lock_);
}

void SRWMutex::lock() noexcept {
	::AcquireSRWLockExclusive(&lock_);
}

void SRWMutex::unlock() noexcept {
	::ReleaseSRWLockExclusive(&lock_);
}

bool SRWMutex::try_lock() noexcept {
	return ::TryAcquireSRWLockExclusive(&lock_);
}
#endif

}
