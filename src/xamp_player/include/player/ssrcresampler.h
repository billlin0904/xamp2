//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <vector>

#include <base/enum.h>
#include <base/align_ptr.h>
#include <base/audiobuffer.h>
#include <player/resampler.h>
#include <player/player.h>

namespace xamp::player {

class XAMP_PLAYER_API SSRCResampler final : public Resampler {
public:
    static const std::string_view VERSION;

    SSRCResampler();

    ~SSRCResampler() override;

    std::string_view GetDescription() const noexcept override;

    void Start(uint32_t input_samplerate, uint32_t num_channels, uint32_t output_samplerate) override;

    bool Process(float const * samples, uint32_t num_sample, AudioBuffer<int8_t> &buffer) override;

    void Flush() override;

    AlignPtr<Resampler> Clone() override;

private:
    class SSRCResamplerImpl;
    AlignPtr<SSRCResamplerImpl> impl_;
};


}

