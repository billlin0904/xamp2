//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>
#include <base/base.h>

namespace xamp::base {

class MemoryMappedFile;

inline constexpr size_t kMaxPreReadFileSize = 8 * 1024 * 1024;

XAMP_BASE_API size_t GetPageSize() noexcept;

XAMP_BASE_API size_t GetPageAlignSize(size_t value) noexcept;

XAMP_BASE_API bool PrefetchFile(std::wstring const &file_name);

XAMP_BASE_API bool PrefetchFile(MemoryMappedFile &file, size_t prefech_size = kMaxPreReadFileSize);

XAMP_BASE_API bool PrefetchMemory(void* adddr, size_t length) noexcept;

#ifdef XAMP_ENABLE_REP_MOVSB
XAMP_BASE_API void MemorySet(void* dest, int32_t c, size_t size) noexcept;
#else
#define MemorySet(dest, c, size) (void) std::memset(dest, c, size)
#endif

#ifdef XAMP_ENABLE_REP_MOVSB
XAMP_BASE_API void MemoryCopy(void* dest, const void* src, size_t size) noexcept;
#else
#define MemoryCopy(dest, src, size) (void) __builtin_memcpy(dest, src, size)
#endif

}

