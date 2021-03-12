//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <valarray>
#include <vector>

#include <base/base.h>
#include <base/math.h>
#include <base/enum.h>
#include <base/audioformat.h>
#include <player/fft.h>

namespace xamp::player {

using namespace xamp::base;

class Spectrum {
public:
	//Spectrum(int32_t num_bands, int32_t min_freq, int32_t max_freq, int32_t frame_size, int32_t sample_rate);

	void Init(AudioFormat const &format);

	void Feed(float const* samples, size_t num_samples);

	float GetSpectralCentroid() const;
private:
	void Process(std::valarray<Complex> const& frames);
	
	std::vector<float> magnitude_;
	FFT fft_;
	AudioFormat format_;
};

}