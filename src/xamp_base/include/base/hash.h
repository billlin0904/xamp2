//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>

namespace xamp::base {

XAMP_BASE_API void MurmurHash3_x64_128(const void* key, const int len, const uint32_t seed, void* out) noexcept;

XAMP_BASE_API void MurmurHash3_x86_32(const void* key, int len, uint32_t seed, void* out) noexcept;

}

