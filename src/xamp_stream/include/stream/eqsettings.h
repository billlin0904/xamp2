//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/stl.h>
#include <stream/stream.h>
#include <base/enum.h>

XAMP_STREAM_NAMESPACE_BEGIN

inline constexpr size_t kEQMaxBand = 10;
inline constexpr auto kDefaultQ = 1.41;
inline constexpr auto kEQMaxDb = 15;
inline constexpr auto kEQMinDb = -15;

XAMP_MAKE_ENUM(EQFilterTypes,
    FT_UNKNOWN,
    FT_LOW_PASS,
    FT_HIGH_PASS,
    FT_HIGH_BAND_PASS,
    FT_HIGH_BAND_PASS_Q,
    FT_NOTCH,
    FT_ALL_PASS,
    FT_ALL_PEAKING_EQ,
    FT_LOW_SHELF,
    FT_LOW_HIGH_SHELF
)

inline constexpr std::array<float, kEQMaxBand> kEqDefaultFrequencies{
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

struct XAMP_STREAM_API EqBandSetting final {
    EQFilterTypes type{ EQFilterTypes::FT_UNKNOWN };
    float frequency{0};
    float gain{0};
    float band_width{0};
    float Q{0};
    float shelf_slope{0};
};

struct XAMP_STREAM_API EqSettings final {
    EqSettings() = default;

    void SetDefault() {
        bands.resize(kEqDefaultFrequencies.size());
        for (auto i = 0; i < kEqDefaultFrequencies.size(); ++i) {
            bands[i].frequency = kEqDefaultFrequencies[i];
            bands[i].band_width = kEqDefaultFrequencies[i] / 2;
            bands[i].Q = kDefaultQ;
        }
    }

    float preamp{ 0 };
    Vector<EqBandSetting> bands;
};

XAMP_STREAM_NAMESPACE_END
