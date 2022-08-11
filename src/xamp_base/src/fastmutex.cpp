#include <base/assert.h>
#include <base/platfrom_handle.h>
#include <base/fastmutex.h>

namespace xamp::base {

class SRWMutex::SRWMutexImpl {
public:
#ifdef XAMP_OS_WIN
    SRWMutexImpl() {
        ::InitializeSRWLock(&lock_);
    }

    void lock() noexcept {
        ::AcquireSRWLockExclusive(&lock_);
    }

    void unlock() noexcept {
        ::ReleaseSRWLockExclusive(&lock_);
    }

    bool try_lock() noexcept {
        return ::TryAcquireSRWLockExclusive(&lock_);
    }

    XAMP_CACHE_ALIGNED(kCacheAlignSize) SRWLOCK lock_;
    uint8_t padding_[kCacheAlignSize - sizeof(lock_)]{ 0 };
#else
    SRWMutexImpl() noexcept {
        auto ret = ::pthread_rwlock_init(&lock_, nullptr);
        XAMP_ASSERT(ret == 0);
	}

    ~SRWMutexImpl() noexcept {
        auto ret = ::pthread_rwlock_destroy(&lock_);
        XAMP_ASSERT(ret == 0);
    }

    void lock() noexcept {
        auto ret = ::pthread_rwlock_wrlock(&lock_);
        XAMP_ASSERT(ret == 0);
    }

    void unlock() noexcept {
        auto ret = ::pthread_rwlock_unlock(&lock_);
        XAMP_ASSERT(ret == 0);
    }

    bool try_lock() noexcept {
        auto ret = ::pthread_rwlock_trywrlock(&lock_);
        if (ret == EBUSY) {
            return false;
        }
        XAMP_ASSERT(ret == 0);
        return true;
    }

    pthread_rwlock_t lock_ = PTHREAD_RWLOCK_INITIALIZER;
#endif
};

XAMP_PIMPL_IMPL(SRWMutex)

SRWMutex::SRWMutex() noexcept
	: impl_(MakeAlign<SRWMutexImpl>()) {
}

void SRWMutex::lock() noexcept {
    return impl_->lock();
}

void SRWMutex::unlock() noexcept {
    return impl_->unlock();
}

bool SRWMutex::try_lock() noexcept {
    return impl_->try_lock();
}

}
