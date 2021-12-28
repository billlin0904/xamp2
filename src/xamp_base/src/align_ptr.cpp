#include <base/base.h>

#ifdef XAMP_OS_WIN
#include <malloc.h>
#endif

#include <base/align_ptr.h>

namespace xamp::base {

#ifndef XAMP_OS_WIN
void* AlignedMalloc(size_t size, size_t aligned_size) noexcept {
    void* p = nullptr;
    return ::posix_memalign(&p, aligned_size, size) == 0 ? p : nullptr;
}

void AlignedFree(void* p) noexcept {
    return ::free(p);
}

void* StackAlloc(size_t size) {
    auto ptr = ::alloca(size);
    return ptr;
}

void StackFree(void* p) {
    (void)p;
}

#else
void* AlignedMalloc(size_t size, size_t aligned_size) noexcept {
    return ::_aligned_malloc(size, aligned_size);
}

void AlignedFree(void* p) noexcept {
    assert(p != nullptr);
    ::_aligned_free(p);
}

void* StackAlloc(size_t size) {
    auto ptr = _malloca(size);
    return ptr;
}

void StackFree(void* p) {
    assert(p != nullptr);
    ::_freea(p);
}
#endif

}
