//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/stream.h>
#include <array>

namespace xamp::stream {

inline constexpr size_t kMaxBand = 10;

inline constexpr std::array<float, kMaxBand> kEQBands{
    31.F,
    62.F,
    125.F,
    250.F,
    500.F,
    1000.F,
    2000.F,
    4000.F,
    8000.F,
    16000.F,
    };

struct XAMP_STREAM_API EQBandSetting final {
    float gain{0};
    float Q{0};
};

struct XAMP_STREAM_API EQSettings final {
    float preamp{0};
    std::array<EQBandSetting, kMaxBand> bands;
};

}
