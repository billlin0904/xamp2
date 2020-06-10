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

using Complex = std::complex<float>;

class XAMP_PLAYER_API FFT {
public:
    FFT();

    void Process(float *block, size_t size, float scale = 1.0);

private:
    void Forward(std::valarray<Complex> &x);

    void Inverse(std::valarray<Complex> &x);
};

}
