//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <cstdint>
#include <base/base.h>

namespace xamp::base {

constexpr uint32_t XAMP_HASH_SEED = 117623077;

XAMP_BASE_API size_t MurmurHash64A(const void* key, size_t len, unsigned int seed = XAMP_HASH_SEED) noexcept;

}

