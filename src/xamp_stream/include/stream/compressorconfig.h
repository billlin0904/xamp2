//=====================================================================================================================
// Copyright (c) 2018-2024 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/stream.h>

XAMP_STREAM_NAMESPACE_BEGIN

struct XAMP_STREAM_API CompressorConfig final {
    CompressorConfig() noexcept
        : gain(0.0)
        , threshold(-1.0)
        , ratio(20)
        , attack(0.1f)
        , release(100) {
    }

    float gain;
    float threshold;
    float ratio;
    float attack;
    float release;
};

XAMP_STREAM_NAMESPACE_END