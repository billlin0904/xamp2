//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <array>
#include <base/audiobuffer.h>
#include <stream/stream.h>

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

class XAMP_STREAM_API Equalizer {
public:
    virtual ~Equalizer() = default;

    virtual void Start(uint32_t num_channels, uint32_t input_samplerate) = 0;

    virtual void SetEQ(uint32_t band, float gain) = 0;

    virtual bool Process(float const * sample_buffer, uint32_t num_samples, AudioBuffer<int8_t>& buffer) = 0;

protected:
    Equalizer() = default;
};

}

