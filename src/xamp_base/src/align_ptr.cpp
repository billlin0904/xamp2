#ifdef _WIN32
#include <malloc.h>
#else
#include <stdlib.h>
#endif

#include <base/align_ptr.h>

namespace xamp::base {

#ifndef _WIN32
void* AlignedAlloc(size_t size, size_t aligned_size) {
    void* p = nullptr;
    auto ret = ::posix_memalign(&p, aligned_size, size);
    assert(ret == 0);
    return p;
}

void AlignedFree(void* p) {
    return ::free(p);
}
#else
void* AlignedAlloc(size_t size, size_t aligned_size) {
    return ::_aligned_malloc(size, aligned_size);
}

void AlignedFree(void* p) {
    ::_aligned_free(p);
}
#endif

}
