//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <cassert>
#include <array>

#include <base/base.h>
#include <base/audioformat.h>
#include <base/assert.h>
#include <base/int24.h>
#include <base/memory.h>
#include <base/simd.h>

XAMP_BASE_NAMESPACE_BEGIN

struct XAMP_BASE_API AudioConvertContext {
    AudioConvertContext();
    size_t in_jump{0};
    size_t out_jump{0};
	float volume_factor{1.0};
    size_t cache_volume{0};
    size_t convert_size{0};
	AudioFormat input_format;
	AudioFormat output_format;
    std::array<size_t, AudioFormat::kMaxChannel> in_offset{};
    std::array<size_t, AudioFormat::kMaxChannel> out_offset{};
};

XAMP_BASE_API AudioConvertContext MakeConvert(AudioFormat const& in_format, AudioFormat const& out_format, size_t convert_size) noexcept;

template <typename T>
void XAMP_VECTOR_CALL ConvertBench(T* XAMP_RESTRICT output, float const* XAMP_RESTRICT input, int32_t float_scale, AudioConvertContext const& context) noexcept {
	XAMP_EXPECTS(output != nullptr);
	XAMP_EXPECTS(input != nullptr);

	const auto* end_input = input + static_cast<ptrdiff_t>(context.convert_size) * context.input_format.GetChannels();

	auto* XAMP_RESTRICT __input = AssumeAligned<kSimdLanes, const float>(input);
	auto* XAMP_RESTRICT __output = AssumeAligned<kSimdLanes, T>(output);

	while (__input != end_input) {
		XAMP_ASSERT(end_input - __input > 0);
		*__output = static_cast<T>(*__input * float_scale);
		++__input;
		++__output;
	}
}

template <PackedFormat InputFormat, PackedFormat OutputFormat>
struct XAMP_BASE_API_ONLY_EXPORT DataConverter {
	// INFO: Only for DSD file
    static void Convert(int8_t* XAMP_RESTRICT output,
                        int8_t const* XAMP_RESTRICT input,
                        AudioConvertContext const& context) noexcept {
		XAMP_EXPECTS(output != nullptr);
		XAMP_EXPECTS(input != nullptr);

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

    static void Convert(int32_t* XAMP_RESTRICT output,
                        float const* XAMP_RESTRICT input,
                        AudioConvertContext const& context) noexcept {
		XAMP_EXPECTS(output != nullptr);
		XAMP_EXPECTS(input != nullptr);

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
#ifdef XAMP_USE_BENCHMAKR
    static void ConvertInt16Bench(int16_t* XAMP_RESTRICT output,
                                  float const* XAMP_RESTRICT input,
                                  AudioConvertContext const& context) noexcept {
		XAMP_ASSERT(output != nullptr);
		XAMP_ASSERT(input != nullptr);

		ConvertBench<int16_t>(output, input, kFloat16Scale, context);
	}

	static void ConvertToInt2432Bench(int32_t* XAMP_RESTRICT output,
		float const* XAMP_RESTRICT input,
		AudioConvertContext const& context) noexcept {
		XAMP_ASSERT(output != nullptr);
		XAMP_ASSERT(input != nullptr);

		const auto* end_input = input + static_cast<ptrdiff_t>(context.convert_size) * context.input_format.GetChannels();
		while (input != end_input) {
			XAMP_ASSERT(end_input - input > 0);
			*output++ = Int24(*input++).To2432Int();
		}
	}

    static void ConvertInt32Bench(int32_t* XAMP_RESTRICT output,
                                  float const* XAMP_RESTRICT input,
                                  AudioConvertContext const& context) noexcept {
		XAMP_ASSERT(output != nullptr);
		XAMP_ASSERT(input != nullptr);

		ConvertBench<int32_t>(output, input, kFloat32Scale, context);
    }
#endif

	static void XAMP_VECTOR_CALL Convert(int16_t* XAMP_RESTRICT output, float const* XAMP_RESTRICT input, AudioConvertContext const& context) noexcept {
		XAMP_EXPECTS(output != nullptr);
		XAMP_EXPECTS(input != nullptr);

		const auto* end_input = input + static_cast<ptrdiff_t>(context.convert_size) * context.input_format.GetChannels();
		const auto scale = SIMD::Set1Ps(kFloat16Scale);

		XAMP_UNLIKELY(!SIMD::IsAligned(input)) {
			const auto unaligned_size = (end_input - input) % kSimdAlignedSize;
			for (auto i = 0; i < unaligned_size; ++i) {
				*output++ = static_cast<int16_t>(*input++);
			}
		}

		auto* XAMP_RESTRICT __input = AssumeAligned<kSimdLanes, const float>(input);
		auto* XAMP_RESTRICT __output = AssumeAligned<kSimdLanes, int16_t>(output);

		while (__input != end_input) {
			XAMP_ASSERT(end_input - __input > 0);
			const auto in = SIMD::LoadPs(__input);
			const auto mul = SIMD::MulPs(in, scale);
			SIMD::F32ToS16(__output, mul);
			__input += kSimdAlignedSize;
			__output += kSimdAlignedSize;
		}
	}

    static void XAMP_VECTOR_CALL Convert(int32_t* XAMP_RESTRICT output,
                        float const* XAMP_RESTRICT input,
                        AudioConvertContext const& context) noexcept {
		XAMP_EXPECTS(output != nullptr);
		XAMP_EXPECTS(input != nullptr);

		const auto* end_input = input + static_cast<ptrdiff_t>(context.convert_size) * context.input_format.GetChannels();
        const auto scale = SIMD::Set1Ps(kFloat32Scale); 

		XAMP_UNLIKELY(!SIMD::IsAligned(input)) {
			const auto unaligned_size = (end_input - input) % kSimdAlignedSize;
			for (auto i = 0; i < unaligned_size; ++i) {
				*output++ = static_cast<int32_t>(*input++);
			}
		}

		auto* XAMP_RESTRICT __input = AssumeAligned<kSimdLanes, const float>(input);
		auto* XAMP_RESTRICT __output = AssumeAligned<kSimdLanes, int32_t>(output);

		while (__input != end_input) {
			XAMP_ASSERT(end_input - __input > 0);
			const auto in = SIMD::LoadPs(__input);
			const auto mul = SIMD::MulPs(in, scale);
			SIMD::F32ToS32(__output, mul);
			__input += kSimdAlignedSize;
			__output += kSimdAlignedSize;
		}
	}

    static void XAMP_VECTOR_CALL ConvertToInt2432(int32_t* XAMP_RESTRICT output,
                                 float const* XAMP_RESTRICT input,
                                 AudioConvertContext const& context) noexcept {
		XAMP_EXPECTS(output != nullptr);
		XAMP_EXPECTS(input != nullptr);

		const auto* end_input = input + static_cast<ptrdiff_t>(context.convert_size) * context.input_format.GetChannels();
		const auto scale = SIMD::Set1Ps(kFloat24Scale);

		XAMP_UNLIKELY(!SIMD::IsAligned(input)) {
			const auto unaligned_size = (end_input - input) % kSimdAlignedSize;
			for (auto i = 0; i < unaligned_size; ++i) {
				*output++ = Int24(*input++).To2432Int();
			}
		}

		auto* XAMP_RESTRICT __input = AssumeAligned<kSimdLanes, const float>(input);
		auto* XAMP_RESTRICT __output = AssumeAligned<kSimdLanes, int32_t>(output);

		while (__input != end_input) {
			XAMP_ASSERT(end_input - __input > 0);
            const auto in = SIMD::LoadPs(__input);
            const auto mul = SIMD::MulPs(in, scale);
            SIMD::F32ToS32<1>(__output, mul);
			__input += kSimdAlignedSize;
			__output += kSimdAlignedSize;
		}
    }
};

XAMP_BASE_NAMESPACE_END
