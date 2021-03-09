//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <complex>
#include <valarray>

#include <base/base.h>
#include <base/align_ptr.h>
#include <base/math.h>
#include <player/player.h>

namespace xamp::player {

using namespace xamp::base;

class FFT final {
public:
    FFT();

    XAMP_PIMPL(FFT)

	void Init(size_t size);

    const std::valarray<Complex>& Forward(float const* data, size_t size);

private:
    class FFTImpl;
    AlignPtr<FFTImpl> impl_;
};

}
