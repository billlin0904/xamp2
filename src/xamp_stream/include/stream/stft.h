//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/buffer.h>
#include <stream/stream.h>
#include <stream/fft.h>

namespace xamp::stream {

class XAMP_STREAM_API STFT {
public:
	STFT(size_t channels, size_t frame_size, size_t shift_size);

	void setWindowType(WindowType type);

	const ComplexValarray& Process(const float* in, size_t length);
private:
	size_t channels_;
	size_t frame_size_;
	size_t shift_size_;
	size_t output_length_;
	FFT fft_;
	Window window_;
	Buffer<float> buf_;
	Buffer<float> in_;
	Buffer<float> out_;
};

}