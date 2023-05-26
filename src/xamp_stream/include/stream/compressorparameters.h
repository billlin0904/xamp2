//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/stream.h>

XAMP_STREAM_NAMESPACE_BEGIN

struct XAMP_STREAM_API CompressorParameters final {
    CompressorParameters() noexcept
        : gain(0.0)
        , threshold(-1.0)
        , ratio(1000)
        , attack(1)
        , release(100) {
    }

    float gain;
    float threshold;
    float ratio;
    float attack;
    float release;
};

XAMP_STREAM_NAMESPACE_END