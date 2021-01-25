//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/enum.h>
#include <base/align_ptr.h>
#include <base/audiobuffer.h>
#include <player/player.h>

namespace xamp::player {

using namespace xamp::base;

class XAMP_PLAYER_API ReplayGain {
public:
	explicit ReplayGain(uint32_t num_channels, uint32_t output_sample_rate);

	XAMP_PIMPL(ReplayGain)

	void Process(float* samples, uint32_t num_sample);

private:
	class ReplayGainImpl;
	AlignPtr<ReplayGainImpl> impl_;
};

}
