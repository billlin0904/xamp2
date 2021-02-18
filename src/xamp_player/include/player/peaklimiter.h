//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/align_ptr.h>
#include <player/player.h>

namespace xamp::player {

using namespace xamp::base;

inline constexpr double kMaxTruePeak = -1.0;

class XAMP_PLAYER_API PeakLimiter {
public:
    PeakLimiter();

    XAMP_PIMPL(PeakLimiter)

    void SetSampleRate(uint32_t output_sample_rate);

    void Setup(float gain, float threshold, float ratio, float attack, float release);

    const std::vector<float>& Process(float const * samples, uint32_t num_samples);

private:
    class PeakLimiterImpl;
    AlignPtr<PeakLimiterImpl> impl_;
};

}

