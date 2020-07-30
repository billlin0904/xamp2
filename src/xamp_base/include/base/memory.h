//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>

#include <base/base.h>

#ifdef XAMP_OS_WIN
#include <base/FastMemcpy.h>
#endif

namespace xamp::base {

class MemoryMappedFile;

XAMP_BASE_API size_t GetPageSize() noexcept;

XAMP_BASE_API size_t GetPageAlignSize(size_t value) noexcept;

XAMP_BASE_API bool PrefactchFile(std::wstring const &file_name);

XAMP_BASE_API bool PrefactchFile(MemoryMappedFile &file);

XAMP_BASE_API bool PrefetchMemory(void* adddr, size_t length) noexcept;

#ifdef XAMP_OS_WIN
#define FastMemcpy(dest, src, size) memcpy_fast(dest, src, size)
#else
#define FastMemcpy(dest, src, size) std::memcpy(dest, src, size)
#endif

}

