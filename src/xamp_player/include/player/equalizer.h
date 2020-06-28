//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <array>
#include <base/audiobuffer.h>
#include <player/player.h>

namespace xamp::player {

using namespace xamp::base;

struct Band {
    float bandwidth{0.0F};
    float center{0.0F};
    float gain{0.0F};
};

static std::array<Band, 10> const kBands {
    Band { 18.F, 50.F,   0.F },
    Band { 18.F, 100.F,  0.F },
    Band { 18.F, 200.F,  0.F },
    Band { 18.F, 400.F,  0.F },
    Band { 18.F, 700.F,  0.F },
    Band { 18.F, 1000.F, 0.F },
    Band { 18.F, 2000.F, 0.F },
    Band { 18.F, 4000.F, 0.F },
    Band { 18.F, 6000.F, 0.F },
    Band { 18.F, 8000.F, 0.F },
};

class XAMP_PLAYER_API Equalizer {
public:
    virtual ~Equalizer() = default;

    virtual bool Process(float const * sample_buffer, uint32_t num_samples, AudioBuffer<int8_t>& buffer) = 0;

protected:
    Equalizer() = default;
};

}

