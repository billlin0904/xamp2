//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <array>
#include <base/audiobuffer.h>
#include <stream/stream.h>
#include <stream/iaudioprocessor.h>

namespace xamp::stream {

using namespace xamp::base;

inline constexpr size_t kMaxBand = 10;

static std::array<float, kMaxBand> const kEQBands{
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

struct XAMP_STREAM_API EQBandSetting {
    float gain{0};
    float Q{0};
};

struct XAMP_STREAM_API EQSettings {
    float preamp{0};
    std::array<EQBandSetting, kMaxBand> bands;
};

class XAMP_NO_VTABLE XAMP_STREAM_API IEqualizer : public IAudioProcessor {
public:
    constexpr static auto Id = std::string_view("FCC73B23-6806-44CD-882D-EA21A3482F51");

    XAMP_BASE_CLASS(IEqualizer)

    virtual void SetEQ(uint32_t band, float gain, float Q) = 0;

    virtual void SetEQ(EQSettings const &settings) = 0;

    virtual void SetPreamp(float preamp) = 0;

    virtual void Disable() = 0;
protected:
    IEqualizer() = default;
};

}
