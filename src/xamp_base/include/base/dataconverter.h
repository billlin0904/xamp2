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
void SSEConvert(TStoreType* XAMP_RESTRICT output, const float* XAMP_RESTRICT input, int32_t float_scale, const AudioConvertContext& context) noexcept {
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
void AVX2Convert(TStoreType* XAMP_RESTRICT output, const float* XAMP_RESTRICT input, int32_t float_scale, const AudioConvertContext& context) noexcept {
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

	static void Convert(int32_t* XAMP_RESTRICT output, const float* XAMP_RESTRICT input, const AudioConvertContext& context) noexcept {
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
struct XAMP_BASE_API_ONLY_EXPORT DataConverter<PackedFormat::INTERLEAVED, PackedFormat::PLANAR> {
	static void Convert(int8_t* XAMP_RESTRICT output, const int8_t* XAMP_RESTRICT input, const AudioConvertContext& context) noexcept {
		XAMP_EXPECTS(output != nullptr);
		XAMP_EXPECTS(input != nullptr);

		const size_t channels = context.input_format.GetChannels();
		const size_t convert_size = context.convert_size;

		const size_t in_jump = channels;
		const size_t out_jump = 1;
		const size_t output_left_offset = 0;
		const size_t output_right_offset = convert_size;

		// mask�]�p�G
	// �ڭ̤@���B�z16 frame (32 bytes)�A�ƦC�� [L0,R0,L1,R1,...,L15,R15]
	// ���n�D��󰸼� index�A�k�n�D���_�� index
	// �Q�� mask ��ܰ��Ʀ�@�����n�D�A�_�Ʀ�@���k�n�D�C
	// �Y���ݭn���줸�եH 0x80 �аO�A�h�Ӧ줸�ձN�Q�M0�C

	// Left channel mask: �����Ʀ� (0,2,4,...,30)�A�N���b��X�e�b 16 bytes�A������(�e16��byte)��0x80�M0
		const __m256i left_shuffle_mask = _mm256_set_epi8(
			(char)0x80, (char)0x80, (char)0x80, (char)0x80,
			(char)0x80, (char)0x80, (char)0x80, (char)0x80,
			(char)0x80, (char)0x80, (char)0x80, (char)0x80,
			(char)0x80, (char)0x80, (char)0x80, (char)0x80,
			30, 28, 26, 24, 22, 20, 18, 16,
			14, 12, 10, 8, 6, 4, 2, 0
		);

		// Right channel mask: ���_�Ʀ� (1,3,5,...,31)
		const __m256i right_shuffle_mask = _mm256_set_epi8(
			(char)0x80, (char)0x80, (char)0x80, (char)0x80,
			(char)0x80, (char)0x80, (char)0x80, (char)0x80,
			(char)0x80, (char)0x80, (char)0x80, (char)0x80,
			(char)0x80, (char)0x80, (char)0x80, (char)0x80,
			31, 29, 27, 25, 23, 21, 19, 17,
			15, 13, 11, 9, 7, 5, 3, 1
		);

		size_t i = 0;
		// �@���B�z16 frame = 32 bytes
		for (; i + 16 <= convert_size; i += 16) {
			__m256i input_values = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(input));

			__m256i left_values = _mm256_shuffle_epi8(input_values, left_shuffle_mask);
			__m256i right_values = _mm256_shuffle_epi8(input_values, right_shuffle_mask);

			// left_values, right_values ���O 256-bit�A���@�b�O0
			// �ڭ̥u�ݭn��C128-bit�Y�� 16��byte���s����
			__m128i left_128 = _mm256_castsi256_si128(left_values);
			__m128i right_128 = _mm256_castsi256_si128(right_values);

			// ��X�쥿�T����m
			_mm_storeu_si128(reinterpret_cast<__m128i*>(output + output_left_offset), left_128);
			_mm_storeu_si128(reinterpret_cast<__m128i*>(output + output_right_offset), right_128);

			input += in_jump * 16; // �e�i16 frame�A1 frame=2 bytes�A�]���e�i32 bytes
			output += out_jump * 16; // ��X�e�i16 bytes�]����16 samples�^
		}

		// �B�z���ݤ���16 frame�������]�жq�B�z�^
		for (; i < convert_size; ++i) {
			output[output_left_offset] = input[0]; // left channel = ����index=0
			output[output_right_offset] = input[1]; // right channel = �_��index=1
			input += in_jump;
			output += out_jump;
		}
	}

	static void Convert(int32_t* XAMP_RESTRICT output, const int32_t* XAMP_RESTRICT input, const AudioConvertContext& context) noexcept {
		XAMP_EXPECTS(output != nullptr);
		XAMP_EXPECTS(input != nullptr);

		const size_t channels = context.input_format.GetChannels();
		XAMP_EXPECTS(channels == 2); // ���]�����n
		const size_t convert_size = context.convert_size;

		const size_t input_jump = context.in_jump;   // �C�� frame �� input �e�i�q(�H int32_t �����)
		const size_t output_jump = context.out_jump; // �C�� frame �� output �e�i�q(�H int32_t �����)

		const size_t input_left_offset = context.in_offset[0];
		const size_t input_right_offset = context.in_offset[1];

		const size_t output_left_offset = context.out_offset[0];
		const size_t output_right_offset = context.out_offset[1];

		// �ڭ̳B�z 4 frames�]8 int32_t�^�Aindex �q 0 �}�l�G
		// int32_0(L), int32_1(R), int32_2(L), int32_3(R), int32_4(L), int32_5(R), int32_6(L), int32_7(R)
		// �C�� int32 �� 4 bytes�A�@ 32 bytes�C
		// �ڭ̭n�ⰸ�� index �� int32 (0,2,4,6) ��� left channel�A
		// �_�� index �� int32 (1,3,5,7) ��� right channel�C
		//
		// bytes �ƦC (�C��int32��4 bytes)�G
		// int32_0: bytes [0..3]
		// int32_1: bytes [4..7]
		// int32_2: bytes [8..11]
		// int32_3: bytes [12..15]
		// int32_4: bytes [16..19]
		// int32_5: bytes [20..23]
		// int32_6: bytes [24..27]
		// int32_7: bytes [28..31]

		// Left channel mask: ���X (0,2,4,6) �o�� int32 �� bytes�G
		// int32_0(0..3), int32_2(8..11), int32_4(16..19), int32_6(24..27)
		// �N��s���b�e 16 bytes (4 int32)�A��l��W0x80�����ΡC
		alignas(32) static const uint8_t left_mask_bytes[32] = {
			0, 1, 2, 3,      // int32_0
			8, 9, 10, 11,    // int32_2
			16,17,18,19,     // int32_4
			24,25,26,27,     // int32_6
			0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80
		};
		// Right channel mask: ���X (1,3,5,7)�G
		// int32_1(4..7), int32_3(12..15), int32_5(20..23), int32_7(28..31)
		alignas(32) static const uint8_t right_mask_bytes[32] = {
			4, 5, 6, 7,       // int32_1
			12,13,14,15,      // int32_3
			20,21,22,23,      // int32_5
			28,29,30,31,      // int32_7
			0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80
		};

		__m256i left_shuffle_mask = _mm256_load_si256(reinterpret_cast<const __m256i*>(left_mask_bytes));
		__m256i right_shuffle_mask = _mm256_load_si256(reinterpret_cast<const __m256i*>(right_mask_bytes));

		size_t i = 0;

		// �@���B�z4��frame
		for (; i + 4 <= convert_size; i += 4) {
			// ���J4��frame * 2channel = 8��int32 = 32 bytes
			__m256i input_values = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(input + i * input_jump));

			__m256i left_values = _mm256_shuffle_epi8(input_values, left_shuffle_mask);
			__m256i right_values = _mm256_shuffle_epi8(input_values, right_shuffle_mask);

			_mm256_storeu_si256(reinterpret_cast<__m256i*>(output + i * output_jump + output_left_offset), left_values);
			_mm256_storeu_si256(reinterpret_cast<__m256i*>(output + i * output_jump + output_right_offset), right_values);
		}

		// ���ݤ���4��frame����ƥH�жq�覡�B�z
		for (; i < convert_size; ++i) {
			output[i * output_jump + output_left_offset] = input[i * input_jump + input_left_offset];
			output[i * output_jump + output_right_offset] = input[i * input_jump + input_right_offset];
		}
	}
};

typedef void (* ConvertCallback)(
	void* XAMP_RESTRICT output, 
	const float* XAMP_RESTRICT input,
	int32_t float_scale,
	const AudioConvertContext& context) noexcept;

inline ConvertCallback ConvertInt16Cb;
inline ConvertCallback Convert2432Cb;
inline ConvertCallback ConvertInt32Cb;

template <typename T, typename TStoreType = T>
void AVX2ConvertWrapper(void* XAMP_RESTRICT output, const float* XAMP_RESTRICT input, int32_t float_scale, const AudioConvertContext& context) noexcept {
	AVX2Convert<T, TStoreType>(reinterpret_cast<TStoreType*>(output), input, float_scale, context);
}

template <typename T, typename TStoreType = T>
void SSEConvertWrapper(void* XAMP_RESTRICT output, const float* XAMP_RESTRICT input, int32_t float_scale, const AudioConvertContext& context) noexcept {
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

	static void Convert(int16_t* XAMP_RESTRICT output, const float* XAMP_RESTRICT input, const AudioConvertContext& context) noexcept {
		ConvertInt16Cb(output, input, kFloat16Scale, context);
	}

	static void Convert(int32_t* XAMP_RESTRICT output, const float* XAMP_RESTRICT input, const AudioConvertContext& context) noexcept {
		ConvertInt32Cb(output, input, kFloat32Scale, context);
	}

	static void ConvertToInt2432(int32_t* XAMP_RESTRICT output, const float* XAMP_RESTRICT input, const AudioConvertContext& context) noexcept {
		Convert2432Cb(output, input, kFloat24Scale, context);
	}
};
#endif

XAMP_BASE_NAMESPACE_END
