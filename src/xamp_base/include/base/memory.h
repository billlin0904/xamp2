//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>
#include <base/base.h>

#if defined(XAMP_OS_WIN) && defined(XAMP_ENABLE_REP_MOVSB)
#include <intrin.h>
#endif

namespace xamp::base {

class MemoryMappedFile;

XAMP_BASE_API size_t GetPageSize() noexcept;

XAMP_BASE_API size_t GetPageAlignSize(size_t value) noexcept;

XAMP_BASE_API bool PrefetchFile(std::wstring const &file_name);

XAMP_BASE_API bool PrefetchFile(MemoryMappedFile &file);

XAMP_BASE_API bool PrefetchMemory(void* adddr, size_t length) noexcept;

#ifdef XAMP_ENABLE_REP_MOVSB
inline void MemorySet(void* dest, int32_t c, size_t size) {
    __stosb(static_cast<unsigned char*>(dest), static_cast<unsigned char>(c), size);
}
#else
#define MemorySet(dest, c, size) (void) std::memset(dest, c, size)
#endif

#ifdef XAMP_ENABLE_REP_MOVSB
inline void MemoryCopy(void* dest, const void* src, size_t size) {
    // ?u??b??????????q???j???G?????A?????????????q???p??Arep movsb ???O?????s?b???}?P?|??P???u???G??????.
    __movsb(static_cast<unsigned char *>(dest), static_cast<const unsigned char*>(src), size);
}
#else
#define MemoryCopy(dest, src, size) (void) std::memcpy(dest, src, size)
#endif

}

