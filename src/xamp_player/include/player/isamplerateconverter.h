//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string_view>

#include <base/align_ptr.h>
#include <base/audiobuffer.h>
#include <base/samplewriter.h>
#include <player/player.h>

namespace xamp::player {

using namespace xamp::base;

class XAMP_PLAYER_API XAMP_NO_VTABLE ISampleRateConverter {
public:
    XAMP_BASE_CLASS(ISampleRateConverter)

    virtual void Start(uint32_t input_sample_rate, uint32_t num_channels, uint32_t output_sample_rate) = 0;

    [[nodiscard]] virtual std::string_view GetDescription() const noexcept = 0;

    [[nodiscard]] virtual bool Process(Buffer<float> const &input, AudioBuffer<int8_t>& buffer) = 0;
	
    [[nodiscard]] virtual bool Process(float const * samples, size_t num_sample, AudioBuffer<int8_t>& buffer) = 0;

    [[nodiscard]] virtual bool Process(float const* samples, size_t num_sample, SampleWriter& writer) = 0;

	virtual void Flush() = 0;

    [[nodiscard]] virtual AlignPtr<ISampleRateConverter> Clone() = 0;

protected:
    ISampleRateConverter() = default;
};

}
