// This is an open source non-commercial project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include <base/base.h>

#ifdef XAMP_OS_WIN
#include <malloc.h>
#else
#include <stdlib.h>
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
#else
void* AlignedMalloc(size_t size, size_t aligned_size) noexcept {
    return ::_aligned_malloc(size, aligned_size);
}

void AlignedFree(void* p) noexcept {
    ::_aligned_free(p);
}
#endif

}
