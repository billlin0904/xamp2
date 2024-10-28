//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/stream.h>

#include <base/memory.h>
#include <base/enum.h>

XAMP_STREAM_NAMESPACE_BEGIN

XAMP_MAKE_ENUM(WindowType,
    NO_WINDOW,
    HAMMING,
    BLACKMAN_HARRIS)

class XAMP_STREAM_API Window final {
public:
    Window();

	XAMP_PIMPL(Window)

    void Init(size_t frame_size, WindowType type = WindowType::BLACKMAN_HARRIS);

    void SetWindowType(WindowType type = WindowType::BLACKMAN_HARRIS);

    void operator()(float* buffer, size_t size) const noexcept;
private:
    class WindowImpl;
    ScopedPtr<WindowImpl> impl_;
};

class XAMP_STREAM_API FFT final {
public:
    FFT();

    XAMP_PIMPL(FFT)

	void Init(size_t frame_size);

    const ComplexValarray& Forward(float const* data, size_t size);

private:
    class FFTImpl;
    ScopedPtr<FFTImpl> impl_;
};

XAMP_STREAM_NAMESPACE_END
