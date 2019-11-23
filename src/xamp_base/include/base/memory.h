//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>

#include <base/base.h>

namespace xamp::base {

XAMP_BASE_API size_t GetPageSize() noexcept;

XAMP_BASE_API size_t GetPageAlignSize(size_t value) noexcept;

XAMP_BASE_API bool PrefetchMemory(void* adddr, size_t length) noexcept;

XAMP_BASE_API XAMP_RESTRICT void* FastMemcpy(void* dest, const void* src, size_t size) noexcept;

XAMP_BASE_API void PrefactchFile(const std::wstring &file_name);

XAMP_BASE_API bool PrefetchMemory(void* adddr, size_t length) noexcept;

}

