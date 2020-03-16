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

using namespace xamp::base;

class XAMP_PLAYER_API SpectrumAnalyser {
public:
	SpectrumAnalyser();

	~SpectrumAnalyser();

	void SetAudioFrameSize(int32_t frame_size);

	void SetSamplingFrequency(int32_t fs);

	const std::vector<float>& GeMagnitude();

	bool Process(AudioBuffer<float> &buffer);
private:
	class SpectrumAnalyserImpl;
	AlignPtr<SpectrumAnalyserImpl> impl_;
};

}

