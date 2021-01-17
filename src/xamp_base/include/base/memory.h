//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>
#include <base/base.h>

#ifdef XAMP_OS_WIN
#include <intrin.h>
#endif

namespace xamp::base {

class MemoryMappedFile;

XAMP_BASE_API size_t GetPageSize() noexcept;

XAMP_BASE_API size_t GetPageAlignSize(size_t value) noexcept;

XAMP_BASE_API bool PrefactchFile(std::wstring const &file_name);

XAMP_BASE_API bool PrefactchFile(MemoryMappedFile &file);

XAMP_BASE_API bool PrefetchMemory(void* adddr, size_t length) noexcept;

#ifdef XAMP_OS_WIN
inline void FastMemcpy(void* dest, const void* src, size_t size) {
    __movsb(static_cast<unsigned char *>(dest), static_cast<const unsigned char*>(src), size);
}
#else
#define FastMemcpy(dest, src, size) (void) std::memcpy(dest, src, size)
#endif
	
}

