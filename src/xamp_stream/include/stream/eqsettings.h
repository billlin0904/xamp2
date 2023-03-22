//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <array>
#include <stream/stream.h>
#include <base/enum.h>

namespace xamp::stream {

inline constexpr size_t kEQMaxBand = 10;
inline constexpr auto kDefaultQ = 1.41;
inline constexpr auto kEQMaxDb = 15;
inline constexpr auto kEQMinDb = -15;

XAMP_MAKE_ENUM(EQFilterTypes,
    FT_LOW_PASS,
    FT_HIGH_PASS,
    FT_HIGH_BAND_PASS,
    FT_HIGH_BAND_PASS_Q,
    FT_NOTCH,
    FT_ALL_PASS,
    FT_ALL_PEAKING_EQ,
    FT_LOW_SHELF,
    FT_LOW_HIGH_SHELF
);

inline constexpr std::array<float, kEQMaxBand> kEQBands{
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
    EQFilterTypes type{ EQFilterTypes::FT_ALL_PASS };
    float gain{0};
    float Q{0};
};

struct XAMP_STREAM_API EQSettings final {
    float preamp{0};
    std::array<EQBandSetting, kEQMaxBand> bands;
};

}
