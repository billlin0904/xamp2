//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <vector>
#include <base/align_ptr.h>
#include <base/audiobuffer.h>
#include <player/player.h>

namespace xamp::player {

class XAMP_PLAYER_API AudioAnalysis {
public:
	AudioAnalysis();

	~AudioAnalysis();

	void SetAudioFrameSize(int32_t frame_size);

	void SetSamplingFrequency(int32_t fs);

	const std::vector<float>& GetMagnitudeSpectrum();

	bool Process(AudioBuffer<float> &buffer);
private:
	class AudioAnalysisImpl;
	AlignPtr<AudioAnalysisImpl> impl_;
};

}
