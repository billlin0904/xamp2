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

namespace xamp::player {

using namespace xamp::base;

class Spectrum {
public:
	//Spectrum(int32_t num_bands, int32_t min_freq, int32_t max_freq, int32_t frame_size, int32_t sample_rate);

	void Init(size_t size);

	void Process(std::valarray<Complex> const& frames);

	float GetSpectralCentroid() const;
private:
	std::vector<float> magnitude_;
};

}