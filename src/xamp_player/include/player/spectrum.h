//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <valarray>
#include <vector>

#include <base/math.h>
#include <base/audiobuffer.h>
#include <base/audioformat.h>
#include <player/fft.h>

namespace xamp::player {

using namespace xamp::base;

class Spectrum {
public:
	void Init(AudioFormat const &format);

	void Feed(float const* samples, size_t num_samples);

	const std::vector<float>& Update();
	
private:
	void Process(std::valarray<Complex> const& frames);

	size_t frame_index_{ 0 };
	size_t last_frame_{ 0 };
	std::vector<float> magnitude_;
	std::vector<float> buffer_;	
	std::vector<float> display_;
	AudioBuffer<float> fifo_;
	FFT fft_;
	AudioFormat format_;
};

}