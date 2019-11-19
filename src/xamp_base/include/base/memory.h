//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string>

#include <base/base.h>

namespace xamp::base {

template <typename T>
constexpr uint8_t HiByte(T val) noexcept {
    return static_cast<uint8_t>(val >> 8);
}

template <typename T>
constexpr uint8_t LowByte(T val) noexcept {
    return static_cast<uint8_t>(val);
}

template <typename T>
constexpr uint32_t HiWord(T val) noexcept {
    return static_cast<uint32_t>(uint16_t(val) >> 16);
}

template <typename T>
constexpr uint32_t LoWord(T val) noexcept {
    return static_cast<uint16_t>(val);
}

XAMP_BASE_API size_t GetPageSize() noexcept;

XAMP_BASE_API size_t GetPageAlignSize(size_t value) noexcept;

XAMP_BASE_API bool PrefetchMemory(void* adddr, size_t length) noexcept;

XAMP_BASE_API XAMP_RESTRICT void* FastMemcpy(void* dest, const void* src, int32_t size) noexcept;

XAMP_BASE_API void PrefactchFile(const std::wstring &file_name);

XAMP_BASE_API bool PrefetchMemory(void* adddr, size_t length) noexcept;

}

