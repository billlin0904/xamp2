#include <base/base.h>

#include <base/assert.h>
#include <base/align_ptr.h>
#include <base/math.h>

#ifdef XAMP_OS_WIN
#include <malloc.h>
#endif

XAMP_BASE_NAMESPACE_BEGIN

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
    XAMP_EXPECTS(IsPowerOfTwo(aligned_size));
    return ::_aligned_malloc(size, aligned_size);
}

void AlignedFree(void* p) noexcept {
    XAMP_EXPECTS(p != nullptr);
    ::_aligned_free(p);
}

void* StackAlloc(size_t size) {
    auto ptr = _malloca(size);
    return ptr;
}

void StackFree(void* p) {
    XAMP_EXPECTS(p != nullptr);
    ::_freea(p);
}
#endif

XAMP_BASE_NAMESPACE_END
