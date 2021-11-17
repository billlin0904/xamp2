//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <cassert>
#include <array>
#include <cmath>
#include <algorithm>

#include <base/base.h>
#include <base/audioformat.h>
#include <base/pvector.h>

namespace xamp::base {

inline constexpr int32_t kFloat16Scale = 32767;
inline constexpr int32_t kFloat24Scale = 8388607;
inline constexpr int32_t kFloat32Scale = 2147483647;
inline constexpr float kMaxFloatSample = 1.0F;
inline constexpr float kMinFloatSample = -1.0F;
	
class Int24 final {
public:
	Int24(float f) noexcept;

	Int24& operator=(int32_t i) noexcept;

	Int24& operator=(float f) noexcept;

	[[nodiscard]] int32_t To2432Int() const noexcept;

	std::array<uint8_t, 3> data;
};

XAMP_ENFORCE_TRIVIAL(Int24)

XAMP_ALWAYS_INLINE Int24::Int24(float f) noexcept {
	*this = f;
}

XAMP_ALWAYS_INLINE Int24& Int24::operator=(float f) noexcept {
	*this = static_cast<int32_t>(f * kFloat24Scale);
	return *this;
}

XAMP_ALWAYS_INLINE Int24& Int24::operator=(int32_t i) noexcept {
	data[0] = reinterpret_cast<uint8_t*>(&i)[0];
	data[1] = reinterpret_cast<uint8_t*>(&i)[1];
	data[2] = reinterpret_cast<uint8_t*>(&i)[2];
	return *this;
}

XAMP_ALWAYS_INLINE int32_t Int24::To2432Int() const noexcept {
	int32_t v{ 0 };
	reinterpret_cast<uint8_t*>(&v)[0] = 0;
	reinterpret_cast<uint8_t*>(&v)[0] = data[0];
	reinterpret_cast<uint8_t*>(&v)[1] = data[1];
	reinterpret_cast<uint8_t*>(&v)[2] = data[2];
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
	const auto* end_input = input + static_cast<ptrdiff_t>(context.convert_size) * context.input_format.GetChannels();
	while (input != end_input) {
		*output = static_cast<T>(*input * float_scale);
		++input;
		++output;
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
		const auto* end_input = input + static_cast<ptrdiff_t>(context.convert_size) * context.input_format.GetChannels();

		const auto scale = F32Set1Ps(kFloat32Scale);
		const auto max_val = F32Set1Ps(kFloat32Scale - 1);
		const auto min_val = F32Set1Ps(-kFloat32Scale);

		auto aligned_bytes = (__F32VEC_SIZE / sizeof(float));
		auto unaligned_size = (end_input - input) % aligned_bytes;

		for (auto i = 0; i < unaligned_size; ++i) {
			*output++ = Int24(*input++).To2432Int();
		}

		while (input != end_input) {
			const auto in = F32VecLoadAligned(input);
			const auto mul = F32MulPs(in, scale);
			const auto clamp = F32MinPs(F32MaxPs(mul, min_val), max_val);
			F32ToS32Aligned(output, clamp);
			input += aligned_bytes;
			output += aligned_bytes;
		}
	}

	static void ConvertToInt2432Std(int32_t* XAMP_RESTRICT output, float const* XAMP_RESTRICT input, AudioConvertContext const& context) noexcept {
		const auto* end_input = input + static_cast<ptrdiff_t>(context.convert_size) * context.input_format.GetChannels();
		while (input != end_input) {
			*output++ = Int24(*input++).To2432Int();
		}
	}

	static void ConvertToInt2432(int32_t* XAMP_RESTRICT output, float const* XAMP_RESTRICT input, AudioConvertContext const& context) noexcept {
		const auto* end_input = input + static_cast<ptrdiff_t>(context.convert_size) * context.input_format.GetChannels();

		const auto scale = F32Set1Ps(kFloat24Scale);
		const auto max_val = F32Set1Ps(kFloat24Scale - 1);
		const auto min_val = F32Set1Ps(-kFloat24Scale);

		auto aligned_bytes = (__F32VEC_SIZE / sizeof(float));
		auto unaligned_size = (end_input - input) % aligned_bytes;

		for (auto i = 0; i < unaligned_size; ++i) {
			*output++ = Int24(*input++).To2432Int();
		}

		while (input != end_input) {
			const auto in = F32VecLoadAligned(input);
			const auto mul = F32MulPs(in, scale);
			const auto clamp = F32MinPs(F32MaxPs(mul, min_val), max_val);
			F32ToS32Aligned<1>(output, clamp);
			input += aligned_bytes;
			output += aligned_bytes;
		}
    }
#endif
};

}
