//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/audiobuffer.h>
#include <player/player.h>

namespace xamp::player {

class XAMP_PLAYER_API XAMP_NO_VTABLE Resampler {
public:
	XAMP_BASE_CLASS(Resampler)

	virtual void Start(int32_t input_samplerate, int32_t num_channels, int32_t output_samplerate, int32_t max_sample) = 0;

	virtual std::string_view GetDescription() const noexcept = 0;

	virtual bool Process(const float* samples, int32_t num_sample, AudioBuffer<int8_t>& buffer) = 0;
protected:
	Resampler() = default;
};

}
