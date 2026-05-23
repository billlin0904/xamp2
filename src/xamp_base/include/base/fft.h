//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/memory.h>
#include <base/enum.h>

#include <complex>
#include <vector>

XAMP_BASE_NAMESPACE_BEGIN

using Complex = std::complex<float>;
using ComplexValarray = std::vector<Complex>;

XAMP_MAKE_ENUM(WindowType,
    NO_WINDOW,
    HAMMING,
    HANN,
    BLACKMAN_HARRIS)

class XAMP_BASE_API Window final {
public:
    Window();

	XAMP_PIMPL(Window)

    void Initialize(size_t frame_size, WindowType type = WindowType::HANN);

    void operator()(float* buffer, size_t size) const ;

private:
    class WindowImpl;
    ScopedPtr<WindowImpl> impl_;
};

class XAMP_BASE_API FFT final {
public:
    FFT();

    XAMP_PIMPL(FFT)

	void Initialize(size_t frame_size);

    const ComplexValarray& Forward(float const* data, size_t size);

private:
    class FFTImpl;
    ScopedPtr<FFTImpl> impl_;
};

XAMP_BASE_NAMESPACE_END
