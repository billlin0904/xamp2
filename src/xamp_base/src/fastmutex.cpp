#include <base/assert.h>
#include <base/platfrom_handle.h>
#include <base/fastmutex.h>
#include <shared_mutex>

XAMP_BASE_NAMESPACE_BEGIN

#if 0

class SRWMutex::SRWMutexImpl {
public:
    SRWMutexImpl() = default;

    void lock() {
        //while (!lock_.try_lock()) {            
        //}
        lock_.lock();
    }

    void unlock() {
        lock_.unlock();
    }

    bool try_lock() {
        return lock_.try_lock();
    }

    std::shared_mutex lock_;
};

XAMP_PIMPL_IMPL(SRWMutex)

SRWMutex::SRWMutex() : impl_(MakeAlign<SRWMutexImpl>()) {
}

void SRWMutex::lock() {
    return impl_->lock();
}

void SRWMutex::unlock() {
    return impl_->unlock();
}

bool SRWMutex::try_lock() {
    return impl_->try_lock();
}

#endif

XAMP_BASE_NAMESPACE_END
