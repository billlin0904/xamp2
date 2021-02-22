//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/align_ptr.h>
#include <player/player.h>

namespace xamp::player {

using namespace xamp::base;

class XAMP_PLAYER_API LoudnessScanner {
public:
	explicit LoudnessScanner(uint32_t sample_rate);

	XAMP_PIMPL(LoudnessScanner)

	void Process(float const * samples, uint32_t num_sample);

	[[nodiscard]] double GetLoudness() const;

	[[nodiscard]] double GetTruePeek() const;
private:
	class LoudnessScannerImpl;
	AlignPtr<LoudnessScannerImpl> impl_;
};

}
