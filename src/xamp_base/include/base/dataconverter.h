//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <cassert>
#include <array>
#include <cmath>
#include <algorithm>

#include <base/base.h>
#include <base/audioformat.h>

#ifdef XAMP_OS_WIN
#include <intrin.h>
#include <xmmintrin.h>
#endif

namespace xamp::base {

inline constexpr int32_t kFloat16Scale = 32767;
inline constexpr int32_t kFloat24Scale = 8388607;
inline constexpr int32_t kFloat32Scale = 2147483647;
inline constexpr float kMaxFloatSample = 1.0F;
inline constexpr float kMinFloatSample = -1.0F;

XAMP_BASE_API void ClampSample(float* f, size_t num_samples) noexcept;

XAMP_BASE_API float ClampSample(float f) noexcept;

#ifdef XAMP_OS_WIN
XAMP_BASE_API float ClampSampleSSE2(float f) noexcept;

XAMP_BASE_API float FloatMaxSSE2(float a, float b) noexcept;
#else
float ClampSampleSSE2(float f) noexcept;

float FloatMax(float a, float b) noexcept;
#endif
	
class Int24 final {
public:
	Int24(float f) noexcept;

	Int24& operator=(int32_t i) noexcept;

	Int24& operator=(float f) noexcept;

	int32_t To2432Int() const noexcept;
protected:
	std::array<uint8_t, 3> c3;
};

XAMP_ENFORCE_TRIVIAL(Int24)

XAMP_ALWAYS_INLINE Int24::Int24(float f) noexcept {
	*this = f;
}

XAMP_ALWAYS_INLINE Int24& Int24::operator=(float f) noexcept {
	*this = static_cast<int32_t>(ClampSampleSSE2(f) * kFloat24Scale);
	return *this;
}

XAMP_ALWAYS_INLINE Int24& Int24::operator=(int32_t i) noexcept {
	c3[0] = reinterpret_cast<uint8_t*>(&i)[0];
	c3[1] = reinterpret_cast<uint8_t*>(&i)[1];
	c3[2] = reinterpret_cast<uint8_t*>(&i)[2];
	return *this;
}

XAMP_ALWAYS_INLINE int32_t Int24::To2432Int() const noexcept {
	int32_t v{ 0 };
	reinterpret_cast<uint8_t*>(&v)[0] = 0;
	reinterpret_cast<uint8_t*>(&v)[0] = c3[0];
	reinterpret_cast<uint8_t*>(&v)[1] = c3[1];
	reinterpret_cast<uint8_t*>(&v)[2] = c3[2];
	return v << 8;
}

struct XAMP_BASE_API AudioConvertContext {
    AudioConvertContext();
    size_t in_jump{0};
    size_t out_jump{0};
	float volume_factor{1.0};
    size_t cache_volume{0};
	AudioFormat input_format;
	AudioFormat output_format;
    size_t convert_size{0};
    std::array<size_t, kMaxChannel> in_offset;
    std::array<size_t, kMaxChannel> out_offset;
};

XAMP_BASE_API AudioConvertContext MakeConvert(AudioFormat const& in_format, AudioFormat const& out_format, size_t convert_size) noexcept;

template <typename T>
void ConvertHelper(T* XAMP_RESTRICT output, float const* XAMP_RESTRICT input, int32_t float_scale, AudioConvertContext const& context) noexcept {
	static_assert(kLoopUnRollingIntCount == 4);

	const auto* end_input = input + static_cast<ptrdiff_t>(context.convert_size) * context.input_format.GetChannels();

	switch ((end_input - input) % kLoopUnRollingIntCount) {
	case 3:
		*output++ = static_cast<T>(*input++ * float_scale);
		[[fallthrough]];
	case 2:
		*output++ = static_cast<T>(*input++ * float_scale);
		[[fallthrough]];
	case 1:
		*output++ = static_cast<T>(*input++ * float_scale);
		[[fallthrough]];
	case 0:
		break;
	}

    auto * XAMP_RESTRICT _input = const_cast<float*>(input);
    auto * XAMP_RESTRICT _output = static_cast<T*>(output);

    while (_input != end_input) {
        _output[0] = static_cast<T>(_input[0] * float_scale);
		_output[1] = static_cast<T>(_input[1] * float_scale);
		_output[2] = static_cast<T>(_input[2] * float_scale);
		_output[3] = static_cast<T>(_input[3] * float_scale);
		_input += kLoopUnRollingIntCount;
		_output += kLoopUnRollingIntCount;
	}
}

inline void Convert2432Helper(int32_t* XAMP_RESTRICT output, float const* XAMP_RESTRICT input, AudioConvertContext const& context) noexcept {
	static_assert(kLoopUnRollingIntCount == 4);

	const auto* end_input = input + static_cast<ptrdiff_t>(context.convert_size) * context.input_format.GetChannels();

	switch ((end_input - input) % kLoopUnRollingIntCount) {
	case 3:
		*output++ = Int24(*input++).To2432Int();
		[[fallthrough]];
	case 2:
		*output++ = Int24(*input++).To2432Int();
		[[fallthrough]];
	case 1:
		*output++ = Int24(*input++).To2432Int();
		[[fallthrough]];
	case 0:
		break;
	}

	while (input != end_input) {
		*output++ = Int24(*input++).To2432Int();
	}
}

template <PackedFormat InputFormat, PackedFormat OutputFormat>
struct XAMP_BASE_API_ONLY_EXPORT DataConverter {
	// INFO: Only for DSD file
	static void Convert(int8_t* XAMP_RESTRICT output, int8_t const* XAMP_RESTRICT input, AudioConvertContext const& context) noexcept {
		const auto output_left_offset = context.out_offset[0];
		const auto output_right_offset = context.out_offset[1];

		const auto input_left_offset = context.in_offset[0];
		const auto input_right_offset = context.in_offset[1];
		
        for (size_t i = 0; i < context.convert_size; ++i) {
			output[output_left_offset] = input[input_left_offset];
			output[output_right_offset] = input[input_right_offset];
			input += context.in_jump;
			output += context.out_jump;
		}
	}

	static void Convert(int32_t* XAMP_RESTRICT output, float const* XAMP_RESTRICT input, AudioConvertContext const& context) noexcept {
		const auto* end_input = input + static_cast<ptrdiff_t>(context.convert_size) * context.input_format.GetChannels();
		
		const auto output_left_offset = context.out_offset[0];
		const auto output_right_offset = context.out_offset[1];

		const auto input_left_offset = context.in_offset[0];
		const auto input_right_offset = context.in_offset[1];

		for (size_t i = 0; i < context.convert_size; ++i) {
			const auto left = input[input_left_offset] * context.volume_factor;
			output[output_left_offset] = static_cast<int32_t>(left * kFloat32Scale);

			const auto right = input[input_right_offset] * context.volume_factor;
			output[output_right_offset] = static_cast<int32_t>(right * kFloat32Scale);
			
			input += context.in_jump;
			output += context.out_jump;
		}
	}
};

template <>
struct XAMP_BASE_API_ONLY_EXPORT DataConverter<PackedFormat::INTERLEAVED, PackedFormat::INTERLEAVED> {
	static void ConvertToInt16(int16_t* XAMP_RESTRICT output, float const* XAMP_RESTRICT input, AudioConvertContext const& context) noexcept {
		ConvertHelper<int16_t>(output, input, kFloat16Scale, context);
	}

	static void ConvertToInt16(int32_t* XAMP_RESTRICT output, float const* XAMP_RESTRICT input, AudioConvertContext const& context) noexcept {
		ConvertHelper<int32_t>(output, input, kFloat32Scale, context);
    }

#ifdef XAMP_OS_WIN
	static void ConvertToInt32(int32_t* XAMP_RESTRICT output, float const* XAMP_RESTRICT input, AudioConvertContext const& context) noexcept {
		static_assert(kLoopUnRollingIntCount == 4);

		const auto* end_input = input + static_cast<ptrdiff_t>(context.convert_size) * context.input_format.GetChannels();

		const auto scale = ::_mm_set1_ps(kFloat24Scale);
		const auto max_val = ::_mm_set1_ps(kFloat24Scale - 1);
		const auto min_val = ::_mm_set1_ps(-kFloat24Scale);

		switch ((end_input - input) % kLoopUnRollingIntCount) {
		case 3:
			*output++ = Int24(*input++).To2432Int();
			[[fallthrough]];
		case 2:
			*output++ = Int24(*input++).To2432Int();
			[[fallthrough]];
		case 1:
			*output++ = Int24(*input++).To2432Int();
			[[fallthrough]];
		case 0:
			break;
		}

		while (input != end_input) {
			const auto in = ::_mm_load_ps(input);
			const auto mul = ::_mm_mul_ps(in, scale);
			const auto clamp = ::_mm_min_ps(_mm_max_ps(mul, min_val), max_val);
			const auto result = ::_mm_cvtps_epi32(clamp);
			::_mm_store_si128(reinterpret_cast<__m128i*>(output), result);
			input += kLoopUnRollingIntCount;
			output += kLoopUnRollingIntCount;
		}
	}

	static void ConvertToInt2432(int32_t* XAMP_RESTRICT output, float const* XAMP_RESTRICT input, AudioConvertContext const& context) noexcept {		
		static_assert(kLoopUnRollingIntCount == 4);

		const auto* end_input = input + static_cast<ptrdiff_t>(context.convert_size) * context.input_format.GetChannels();

		const auto scale = ::_mm_set1_ps(kFloat24Scale);
		const auto max_val = ::_mm_set1_ps(kFloat24Scale - 1);
		const auto min_val = ::_mm_set1_ps(-kFloat24Scale);

		switch ((end_input - input) % kLoopUnRollingIntCount) {
		case 3:
			*output++ = Int24(*input++).To2432Int();
			[[fallthrough]];
		case 2:
			*output++ = Int24(*input++).To2432Int();
			[[fallthrough]];
		case 1:
			*output++ = Int24(*input++).To2432Int();
			[[fallthrough]];
		case 0:
			break;
		}

		while (input != end_input) {
			const auto in = ::_mm_load_ps(input);
			const auto mul = ::_mm_mul_ps(in, scale);
			const auto clamp = ::_mm_min_ps(_mm_max_ps(mul, min_val), max_val);
			const auto result = ::_mm_cvtps_epi32(clamp);
			const auto shift = ::_mm_slli_si128(result, 1);
			::_mm_store_si128(reinterpret_cast<__m128i*>(output), shift);
			input += kLoopUnRollingIntCount;
			output += kLoopUnRollingIntCount;
		}
    }
#endif
};

}
