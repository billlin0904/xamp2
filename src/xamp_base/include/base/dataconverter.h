//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <cassert>
#include <array>
#include <cmath>

#include <base/base.h>
#include <base/audioformat.h>

#ifdef XAMP_OS_WIN
#include <intrin.h>
#include <xmmintrin.h>
#endif

namespace xamp::base {

inline constexpr int32_t kFloat16Scaler = 32767;
inline constexpr int32_t kFloat24Scaler = 8388607;
inline constexpr int32_t kFloat32Scaler = 2147483647;

XAMP_ALWAYS_INLINE float Clip(float f) noexcept {
	auto x = f;
	x = ((x < -1.0) ? -1.0 : ((x > 1.0) ? 1.0 : x));	
	return x;
}

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
	*this = static_cast<int32_t>(Clip(f) * kFloat24Scaler);	
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
void ConvertHelper(T* XAMP_RESTRICT output, float const* XAMP_RESTRICT input, float FloatScaler, AudioConvertContext const& context) noexcept {
	const auto* end_input = input + ptrdiff_t(context.convert_size) * context.input_format.GetChannels();

	switch ((end_input - input) % kLoopUnRollingIntCount) {
	case 3:
		*output++ = static_cast<T>(Clip(*input++) * FloatScaler);
		[[fallthrough]];
	case 2:
		*output++ = static_cast<T>(Clip(*input++) * FloatScaler);
		[[fallthrough]];
	case 1:
		*output++ = static_cast<T>(Clip(*input++) * FloatScaler);
		[[fallthrough]];
	case 0:
		break;
	}

    auto * XAMP_RESTRICT _input = (float*) XAMP_ALIGN_ASSUME_ALIGNED(input, kMallocAlignSize);
    auto * XAMP_RESTRICT _output = (T*) XAMP_ALIGN_ASSUME_ALIGNED(output, kMallocAlignSize);

    while (_input != end_input) {
        *_output++ = static_cast<T>(Clip(*_input++) * FloatScaler);
	}
}

inline void Convert2432Helper(int32_t* XAMP_RESTRICT output, float const* XAMP_RESTRICT input, AudioConvertContext const& context) noexcept {
	const auto* end_input = input + ptrdiff_t(context.convert_size) * context.input_format.GetChannels();

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
        for (size_t i = 0; i < context.convert_size; ++i) {
			output[context.out_offset[0]] = input[context.in_offset[0]];
			output[context.out_offset[1]] = input[context.in_offset[1]];
			input += context.in_jump;
			output += context.out_jump;
		}
	}

	static void Convert(int32_t* XAMP_RESTRICT output, float const* XAMP_RESTRICT input, AudioConvertContext const& context) noexcept {
		const auto output_left_offset = context.out_offset[0];
		const auto output_right_offset = context.out_offset[1];

		const auto input_left_offset = context.in_offset[0];
		const auto input_right_offset = context.in_offset[1];

		for (size_t i = 0; i < context.convert_size; ++i) {
			const auto left = Clip(input[input_left_offset] * context.volume_factor);
			output[output_left_offset] = static_cast<int32_t>(left * kFloat32Scaler);

			const auto right = Clip(input[input_right_offset] * context.volume_factor);
			output[output_right_offset] = static_cast<int32_t>(right * kFloat32Scaler);
			
			input += context.in_jump;
			output += context.out_jump;
		}
	}
};

static void FastDSDConvert(int8_t* XAMP_RESTRICT output, int8_t const* XAMP_RESTRICT input, AudioConvertContext const& context) noexcept {
	for (auto i = 0; i != context.convert_size; ++i) {
		output[(i * 2) + 0] = input[i + (0 * context.convert_size)];
		output[(i * 2) + 1] = input[i + (1 * context.convert_size)];
	}
}

template <>
struct XAMP_BASE_API_ONLY_EXPORT DataConverter<PackedFormat::INTERLEAVED, PackedFormat::INTERLEAVED> {
	static void ConvertToInt16(int16_t* XAMP_RESTRICT output, float const* XAMP_RESTRICT input, AudioConvertContext const& context) noexcept {
		ConvertHelper<int16_t>(output, input, kFloat16Scaler, context);
	}

	static void ConvertToInt16(int32_t* XAMP_RESTRICT output, float const* XAMP_RESTRICT input, AudioConvertContext const& context) noexcept {
		ConvertHelper<int32_t>(output, input, kFloat32Scaler, context);
    }

#ifdef XAMP_OS_WIN
	static void ConvertToInt32(int32_t* XAMP_RESTRICT output, float const* XAMP_RESTRICT input, AudioConvertContext const& context) noexcept {
		const auto* end_input = input + ptrdiff_t(context.convert_size) * context.input_format.GetChannels();

		const auto scale = ::_mm_set1_ps(kFloat24Scaler);
		const auto max_val = ::_mm_set1_ps(kFloat24Scaler - 1);
		const auto min_val = ::_mm_set1_ps(-kFloat24Scaler);

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
			input += 4;
			output += 4;
		}
	}

	static void ConvertToInt2432(int32_t* XAMP_RESTRICT output, float const* XAMP_RESTRICT input, AudioConvertContext const& context) noexcept {		
		const auto* end_input = input + ptrdiff_t(context.convert_size) * context.input_format.GetChannels();

		const auto scale = ::_mm_set1_ps(kFloat24Scaler);
		const auto max_val = ::_mm_set1_ps(kFloat24Scaler - 1);
		const auto min_val = ::_mm_set1_ps(-kFloat24Scaler);

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
			input += 4;
			output += 4;
		}
    }
#endif
};

}
