//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <complex>
#include <valarray>
#include <cmath>

#include <player/player.h>

namespace xamp::player {

inline constexpr float PI = 3.141592741F;

class XAMP_PLAYER_API FFT {
public:
	FFT() = default;

	void Forward(std::valarray<std::complex<float>> &x);

	void Inverse(std::valarray<std::complex<float>> &x);
};

}