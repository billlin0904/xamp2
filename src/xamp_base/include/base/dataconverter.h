//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

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

#ifdef XAMP_OS_WIN
template <typename T, typename TStoreType = T>
void XAMP_VECTOR_CALL SSEConvert(TStoreType* XAMP_RESTRICT output, const float* XAMP_RESTRICT input, int32_t float_scale, const AudioConvertContext& context) noexcept {
	XAMP_EXPECTS(output != nullptr);
	XAMP_EXPECTS(input != nullptr);

	const auto* end_input = input + static_cast<ptrdiff_t>(context.convert_size) * context.input_format.GetChannels();

	auto* XAMP_RESTRICT __input = AssumeAligned<kSSESimdLanes, const float>(input);
	auto* XAMP_RESTRICT __output = AssumeAligned<kSSESimdLanes, TStoreType>(output);

	const __m128 scale = _mm_set1_ps(static_cast<float>(float_scale));

	while (__input + 4 <= end_input) {
		XAMP_ASSERT(end_input - __input > 0);

		__m128 input_values = _mm_load_ps(__input);
		__m128 scaled_values = _mm_mul_ps(input_values, scale);

		if constexpr (std::is_same_v<T, int32_t>) {
			__m128i output_values = _mm_cvtps_epi32(scaled_values);
			_mm_store_si128(reinterpret_cast<__m128i*>(__output), output_values);
			__output += 4;
		}
		else if constexpr (sizeof(T) == 3) {
			__m128i output_values = _mm_cvtps_epi32(scaled_values);
			alignas(16) int32_t temp_output[4];
			_mm_store_si128(reinterpret_cast<__m128i*>(temp_output), output_values);
			__m128i temp_output_values = _mm_load_si128(reinterpret_cast<const __m128i*>(temp_output));
			__m128i shifted_values = _mm_slli_si128(temp_output_values, 1);
			_mm_store_si128(reinterpret_cast<__m128i*>(__output), shifted_values);
			__output += 4;
		}
		else if constexpr (std::is_same_v<T, short>) {
			__m128i output_values = _mm_cvtps_epi32(scaled_values);
			__m128i packed_values = _mm_packs_epi32(output_values, output_values);
			_mm_storel_epi64(reinterpret_cast<__m128i*>(__output), packed_values);
			__output += 4;
		}
		else {
			__m128 output_values = scaled_values;
			_mm_store_ps(reinterpret_cast<float*>(__output), output_values);
			__output += 4;
		}

		__input += 4;
	}

	while (__input != end_input) {
		XAMP_ASSERT(end_input - __input > 0);
		if constexpr (sizeof(T) == 3) {
			int32_t temp = static_cast<int32_t>(*__input * float_scale) << 8;
			*reinterpret_cast<int32_t*>(__output) = temp;
			__output += 4;
		}
		else if constexpr (std::is_same_v<T, short>) {
			*__output = static_cast<short>(*__input * float_scale);
			++__output;
		}
		else {
			*__output = static_cast<T>(*__input * float_scale);
			++__output;
		}
		++__input;
	}
}

template <typename T, typename TStoreType = T>
void XAMP_VECTOR_CALL AVX2Convert(TStoreType* XAMP_RESTRICT output, const float* XAMP_RESTRICT input, int32_t float_scale, const AudioConvertContext& context) noexcept {
	XAMP_EXPECTS(output != nullptr);
	XAMP_EXPECTS(input != nullptr);

	const auto* end_input = input + static_cast<ptrdiff_t>(context.convert_size) * context.input_format.GetChannels();

	auto* XAMP_RESTRICT __input = AssumeAligned<kAVX2SimdLanes, const float>(input);
	auto* XAMP_RESTRICT __output = AssumeAligned<kAVX2SimdLanes, TStoreType>(output);

	const __m256 scale = _mm256_set1_ps(static_cast<float>(float_scale));

	while (__input + 8 <= end_input) {
		XAMP_ASSERT(end_input - __input > 0);

		__m256 input_values = _mm256_load_ps(__input);
		__m256 scaled_values = _mm256_mul_ps(input_values, scale);

		if constexpr (std::is_same_v<T, int32_t>) {
			__m256i output_values = _mm256_cvtps_epi32(scaled_values);
			_mm256_store_si256(reinterpret_cast<__m256i*>(__output), output_values);
			__output += 8;
		}
		else if constexpr (sizeof(T) == 3) {
			__m256i output_values = _mm256_cvtps_epi32(scaled_values);
			alignas(32) int32_t temp_output[8];
			_mm256_store_si256(reinterpret_cast<__m256i*>(temp_output), output_values);
			__m256i temp_output_values = _mm256_load_si256(reinterpret_cast<const __m256i*>(temp_output));
			__m256i shifted_values = _mm256_slli_si256(temp_output_values, 1);
			_mm256_store_si256(reinterpret_cast<__m256i*>(__output), shifted_values);
			__output += 8;
		}
		else if constexpr (std::is_same_v<T, short>) {
			__m256i output_values = _mm256_cvtps_epi32(scaled_values);
			__m256i packed_values = _mm256_packs_epi32(output_values, output_values);
			packed_values = _mm256_permute4x64_epi64(packed_values, 0xD8);
			_mm_store_si128(reinterpret_cast<__m128i*>(__output), _mm256_castsi256_si128(packed_values));
			__output += 8;
		}
		else {
			__m256 output_values = scaled_values;
			_mm256_store_ps(reinterpret_cast<float*>(__output), output_values);
			__output += 8;
		}

		__input += 8;
	}

	while (__input != end_input) {
		XAMP_ASSERT(end_input - __input > 0);
		if constexpr (sizeof(T) == 3) {
			int32_t temp = static_cast<int32_t>(*__input * float_scale) << 8;
			*reinterpret_cast<int32_t*>(__output) = temp;
			__output += 1;
		}
		else if constexpr (std::is_same_v<T, short>) {
			*__output = static_cast<short>(*__input * float_scale);
			++__output;
		}
		else {
			*__output = static_cast<T>(*__input * float_scale);
			++__output;
		}
		++__input;
	}
}

template <PackedFormat InputFormat, PackedFormat OutputFormat>
struct XAMP_BASE_API_ONLY_EXPORT DataConverter {
	// INFO: Only for DSD file
    static void Convert(int8_t* XAMP_RESTRICT output, const int8_t* XAMP_RESTRICT input, const AudioConvertContext& context) noexcept {
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

    static void Convert(int32_t* XAMP_RESTRICT output, const float * XAMP_RESTRICT input, const AudioConvertContext& context) noexcept {
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

typedef void (XAMP_VECTOR_CALL* ConvertCallback)(
	void* XAMP_RESTRICT output, 
	const float* XAMP_RESTRICT input,
	int32_t float_scale,
	const AudioConvertContext& context) noexcept;

inline ConvertCallback ConvertInt16Cb;
inline ConvertCallback Convert2432Cb;
inline ConvertCallback ConvertInt32Cb;

template <typename T, typename TStoreType = T>
void XAMP_VECTOR_CALL AVX2ConvertWrapper(void* XAMP_RESTRICT output, const float* XAMP_RESTRICT input, int32_t float_scale, const AudioConvertContext& context) noexcept {
	AVX2Convert<T, TStoreType>(reinterpret_cast<TStoreType*>(output), input, float_scale, context);
}

template <typename T, typename TStoreType = T>
void XAMP_VECTOR_CALL SSEConvertWrapper(void* XAMP_RESTRICT output, const float* XAMP_RESTRICT input, int32_t float_scale, const AudioConvertContext& context) noexcept {
	SSEConvert<T, TStoreType>(reinterpret_cast<TStoreType*>(output), input, float_scale, context);
}

template <>
struct XAMP_BASE_API_ONLY_EXPORT DataConverter<PackedFormat::INTERLEAVED, PackedFormat::INTERLEAVED> {
	static void Initial() {
		if (!SIMD::IsCPUSupportAVX2()) {
			ConvertInt16Cb = SSEConvertWrapper<int16_t>;
			Convert2432Cb = SSEConvertWrapper<Int24, int32_t>;
			ConvertInt32Cb = SSEConvertWrapper<int32_t>;
		}
		else {
			ConvertInt16Cb = AVX2ConvertWrapper<int16_t>;
			Convert2432Cb = AVX2ConvertWrapper<Int24, int32_t>;
			ConvertInt32Cb = AVX2ConvertWrapper<int32_t>;
		}
	}

	static void XAMP_VECTOR_CALL Convert(int16_t* XAMP_RESTRICT output, const float* XAMP_RESTRICT input, const AudioConvertContext& context) noexcept {
		ConvertInt16Cb(output, input, kFloat16Scale, context);
	}

	static void XAMP_VECTOR_CALL Convert(int32_t* XAMP_RESTRICT output, const float* XAMP_RESTRICT input, const AudioConvertContext& context) noexcept {
		ConvertInt32Cb(output, input, kFloat32Scale, context);
	}

	static void XAMP_VECTOR_CALL ConvertToInt2432(int32_t* XAMP_RESTRICT output, const float* XAMP_RESTRICT input, const AudioConvertContext& context) noexcept {
		Convert2432Cb(output, input, kFloat24Scale, context);
	}
};
#endif

XAMP_BASE_NAMESPACE_END
