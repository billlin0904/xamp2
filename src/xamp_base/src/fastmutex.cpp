#include <base/assert.h>
#include <base/platfrom_handle.h>
#include <base/fastmutex.h>
#include <shared_mutex>

XAMP_BASE_NAMESPACE_BEGIN

class SRWMutex::SRWMutexImpl {
public:
    SRWMutexImpl() = default;

    void lock() noexcept {
        lock_.lock();
    }

    void unlock() noexcept {
        lock_.unlock();
    }

    bool try_lock() noexcept {
        return lock_.try_lock();
    }

    std::shared_mutex lock_;
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

XAMP_BASE_NAMESPACE_END
