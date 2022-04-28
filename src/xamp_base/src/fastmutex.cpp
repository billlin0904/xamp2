#include <base/assert.h>
#include <base/fastmutex.h>

namespace xamp::base {

#ifdef XAMP_OS_WIN
SRWMutex::SRWMutex() noexcept {
	::InitializeSRWLock(&lock_);
}

SRWMutex::~SRWMutex() noexcept {

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
#else
SRWMutex::SRWMutex() noexcept {
    auto ret = ::pthread_rwlock_init(&lock_, nullptr);
    XAMP_ASSERT(ret == 0);
}

SRWMutex::~SRWMutex() noexcept {
    auto ret = ::pthread_rwlock_destroy(&lock_);
    XAMP_ASSERT(ret == 0);
}

void SRWMutex::lock() noexcept {
    auto ret = ::pthread_rwlock_wrlock(&lock_);
    XAMP_ASSERT(ret == 0);
}

void SRWMutex::unlock() noexcept {
    auto ret = ::pthread_rwlock_unlock(&lock_);
    XAMP_ASSERT(ret == 0);
}

bool SRWMutex::try_lock() noexcept {
    auto ret = ::pthread_rwlock_trywrlock(&lock_);
    if (ret == EBUSY) {
        return false;
    }
    XAMP_ASSERT(ret == 0);
    return true;
}
#endif

}
