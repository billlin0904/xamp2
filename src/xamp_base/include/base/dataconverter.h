//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <array>
#include <cassert>
#include <cmath>

#include <base/base.h>
#include <base/audioformat.h>

#ifdef XAMP_OS_WIN
#include <intrin.h>
#include <xmmintrin.h>
#endif

namespace xamp::base {

// See: http://blog.bjornroche.com/2009/12/int-float-int-its-jungle-out-there.html
constexpr int32_t kXampFloat16Scaler = 0x8000;
constexpr int32_t kXampFloat24Scaler = 0x800000;
constexpr int64_t kXampFloat32Scaler = 0x7FFFFFBE;

class int24_t final {
public:
	int24_t(float f) noexcept;

	int24_t& operator=(int32_t i) noexcept;

	int24_t& operator=(float f) noexcept;

	int32_t to_2432int() const noexcept;
protected:
	std::array<uint8_t, 3> c3;
};

XAMP_ENFORCE_TRIVIAL(int24_t)

XAMP_ALWAYS_INLINE int24_t::int24_t(float f) noexcept {
	*this = f;
}

XAMP_ALWAYS_INLINE int24_t& int24_t::operator=(float f) noexcept {
	*this = static_cast<int32_t>(std::rint(f * kXampFloat24Scaler));
	return *this;
}

XAMP_ALWAYS_INLINE int24_t& int24_t::operator=(int32_t i) noexcept {
	c3[0] = reinterpret_cast<uint8_t*>(&i)[0];
	c3[1] = reinterpret_cast<uint8_t*>(&i)[1];
	c3[2] = reinterpret_cast<uint8_t*>(&i)[2];
	return *this;
}

XAMP_ALWAYS_INLINE int32_t int24_t::to_2432int() const noexcept {
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
    std::array<size_t, XAMP_MAX_CHANNEL> in_offset;
    std::array<size_t, XAMP_MAX_CHANNEL> out_offset;
};

XAMP_BASE_API AudioConvertContext MakeConvert(const AudioFormat& in_format, const AudioFormat& out_format, size_t convert_size) noexcept;

template <InterleavedFormat InputFormat, InterleavedFormat OutputFormat>
struct DataConverter {
	// INFO: Only for DSD file
	static XAMP_RESTRICT int8_t* Convert(int8_t* output, const int8_t* input, const AudioConvertContext& context) noexcept {
        for (size_t i = 0; i < context.convert_size; ++i) {
			output[context.out_offset[0]] = input[context.in_offset[0]];
			output[context.out_offset[1]] = input[context.in_offset[1]];
			input += context.in_jump;
			output += context.out_jump;
		}
		return output;
	}

	static XAMP_RESTRICT int16_t* Convert(int16_t* output, const float* input, const AudioConvertContext& context) noexcept {
        for (size_t i = 0; i < context.convert_size; ++i) {
			output[context.out_offset[0]] = static_cast<int16_t>(input[context.in_offset[0]] * kXampFloat16Scaler * context.volume_factor);
			output[context.out_offset[1]] = static_cast<int16_t>(input[context.in_offset[1]] * kXampFloat16Scaler * context.volume_factor);
			input += context.in_jump;
			output += context.out_jump;
		}
		return output;
	}

	static XAMP_RESTRICT int24_t* Convert(int24_t* output, const float* input, const AudioConvertContext& context) noexcept {
        for (size_t i = 0; i < context.convert_size; ++i) {
			output[context.out_offset[0]] = static_cast<int32_t>(input[context.in_offset[0]] * kXampFloat24Scaler);
			output[context.out_offset[1]] = static_cast<int32_t>(input[context.in_offset[1]] * kXampFloat24Scaler);
			input += context.in_jump;
			output += context.out_jump;
		}
		return output;
	}

	static XAMP_RESTRICT int32_t* Convert(int32_t* output, const float* input, const AudioConvertContext& context) noexcept {
		const auto output_left_offset = context.out_offset[0];
		const auto output_right_offset = context.out_offset[1];

		const auto input_left_offset = context.in_offset[0];
		const auto input_right_offset = context.in_offset[1];

		for (int32_t i = 0; i < context.convert_size; ++i) {
			output[output_left_offset] = static_cast<int32_t>((input[input_left_offset] * kXampFloat32Scaler) * context.volume_factor);
			output[output_right_offset] = static_cast<int32_t>((input[input_right_offset] * kXampFloat32Scaler) * context.volume_factor);
			input += context.in_jump;
			output += context.out_jump;
		}
		return output;
	}

    static XAMP_RESTRICT float* Convert(float* output, const float* input, const AudioConvertContext& context) noexcept {
        const auto output_left_offset = context.out_offset[0];
        const auto output_right_offset = context.out_offset[1];

        const auto input_left_offset = context.in_offset[0];
        const auto input_right_offset = context.in_offset[1];

        for (size_t i = 0; i < context.convert_size; ++i) {
            output[output_left_offset] = input[input_left_offset] * context.volume_factor;
            output[output_right_offset] = input[input_right_offset] * context.volume_factor;
            input += context.in_jump;
            output += context.out_jump;
        }
        return output;
    }
};

template <typename T, int64_t FloatScaler>
XAMP_RESTRICT T* ConvertHelper(T *output, const float* input, const AudioConvertContext& context) noexcept {
	const auto* end_input = input + static_cast<size_t>(context.convert_size) * context.input_format.GetChannels();

	while (input != end_input) {
		*output++ = T(*input++ * FloatScaler);
	}
	return output;
}

inline XAMP_RESTRICT int32_t* Convert2432Helper(int32_t* output, const float* input, const AudioConvertContext& context) noexcept {
	const auto* end_input = input + static_cast<size_t>(context.convert_size) * context.input_format.GetChannels();

	while (input != end_input) {
		*output++ = int24_t(*input++).to_2432int();
	}
	return output;
}

template <>
struct DataConverter<InterleavedFormat::INTERLEAVED, InterleavedFormat::INTERLEAVED> {
	static XAMP_RESTRICT int16_t* ConvertToInt16(int16_t* output, const float* input, const AudioConvertContext& context) noexcept {
		return ConvertHelper<int16_t, kXampFloat16Scaler>(output, input, context);
	}

	static XAMP_RESTRICT int32_t* ConvertToInt16(int32_t* output, const float* input, const AudioConvertContext& context) noexcept {
		return ConvertHelper<int32_t, kXampFloat32Scaler>(output, input, context);
    }

#ifdef XAMP_OS_WIN
	static XAMP_RESTRICT int32_t* ConvertToInt2432(int32_t* output, const float* input, const AudioConvertContext& context) noexcept {
		const auto* end_input = input + static_cast<size_t>(context.convert_size) * context.input_format.GetChannels();

		auto scale = _mm_set1_ps(kXampFloat24Scaler);
		auto max_val = _mm_set1_ps(kXampFloat24Scaler - 1);
		auto min_val = _mm_set1_ps(-kXampFloat24Scaler);

		switch ((end_input - input) % 4) {
		case 3:
			*output++ = int24_t(*input++).to_2432int();
		case 2:
			*output++ = int24_t(*input++).to_2432int();
		case 1:
			*output++ = int24_t(*input++).to_2432int();
		case 0:
			break;
		}

		while (input != end_input) {
			auto in = _mm_load_ps(input);
			auto mul = _mm_mul_ps(in, scale);
			auto clamp = _mm_min_ps(_mm_max_ps(mul, min_val), max_val);
			auto result = _mm_cvtps_epi32(clamp);
			auto shift = _mm_slli_si128(result, 1);
			_mm_store_si128(reinterpret_cast<__m128i*>(output), shift);
			input += 4;
			output += 4;
		}

		return output;
    }
#endif
};

}
