//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <complex>
#include <valarray>

#include <base/align_ptr.h>
#include <base/enum.h>
#include <stream/stream.h>

namespace xamp::stream {

using Complex = std::complex<float>;
using ComplexValarray = std::valarray<Complex>;

MAKE_XAMP_ENUM(WindowType,
    HANN,
    HAMMING)

class XAMP_STREAM_API Window final {
public:
    Window();

	XAMP_PIMPL(Window)

    void Init(WindowType type = WindowType::HAMMING);

    float operator()(size_t i, size_t N) const;
private:
    class WindowImpl;
    AlignPtr<WindowImpl> impl_;
};

class XAMP_STREAM_API FFT final {
public:
    FFT();

    XAMP_PIMPL(FFT)

	void Init(size_t size);

    const ComplexValarray& Forward(float const* data, size_t size);

private:
    class FFTImpl;
    AlignPtr<FFTImpl> impl_;
};

}

