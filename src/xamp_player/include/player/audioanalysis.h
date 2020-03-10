//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <vector>
#include <base/align_ptr.h>
#include <player/player.h>

namespace xamp::player {

class XAMP_PLAYER_API AudioAnalysis {
public:
	AudioAnalysis(int32_t frame_size, int32_t samplerate);

	~AudioAnalysis();

	const std::vector<float>& GetMagnitudeSpectrum();

	void Process(const std::vector<float>& frame);
private:
	class AudioAnalysisImpl;
	AlignPtr<AudioAnalysisImpl> impl_;
};

}

