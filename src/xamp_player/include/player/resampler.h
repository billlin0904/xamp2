//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <string_view>

#include <base/audiobuffer.h>
#include <player/player.h>

namespace xamp::player {

using namespace xamp::base;

class XAMP_PLAYER_API XAMP_NO_VTABLE Resampler {
public:
	virtual ~Resampler() = default;

    virtual void Start(uint32_t input_samplerate, uint32_t num_channels, uint32_t output_samplerate, uint32_t max_sample) = 0;

	virtual std::string_view GetDescription() const noexcept = 0;

    virtual bool Process(float const * samples, uint32_t num_sample, AudioBuffer<int8_t>& buffer) = 0;

	virtual void Flush() = 0;

protected:
	Resampler() = default;
};

}
