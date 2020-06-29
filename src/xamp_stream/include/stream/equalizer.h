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

struct Band { 
    float center{0.0F};
    float bandwidth{ 0.0F };
};

static std::array<Band, kMaxBand> const kEQBands {
    Band { 32.F,    3.F },
    Band { 64.F,    4.F },
    Band { 125.F,   5.F },
    Band { 250.F,   6.F },
    Band { 500.F,   8.F },
    Band { 1000.F,  10.F },
    Band { 2000.F,  12.F },
    Band { 4000.F,  12.F },
    Band { 8000.F,  18.F },
    Band { 16000.F, 36.F },
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

