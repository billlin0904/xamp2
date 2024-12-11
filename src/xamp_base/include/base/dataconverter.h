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
	// 僅使用 `_mm256_permutevar8x32_epi32` 無法完成 byte-level 重組，原因如下：
	// 1. 原始需求是 byte-level 重組：
	//    interleaved 格式 ([L0,R0,L1,R1,...]) 需分離成 planar 格式 ([L0,L1,...][R0,R1,...])，
	//    需要對單個 byte 進行精確控制，包含跨 128-bit lane 的搬移。
	// 2. `_mm256_permutevar8x32_epi32` 的限制：
	//    此指令以 32-bit (dword) 為單位進行重組，只能重排 dword，無法重排 byte，
	//    因此無法分離左右聲道的 byte 資料。
	// 3. 跨 lane 的問題：
	//    `_mm256_shuffle_epi8` 能處理 byte-level 重組，但限制在 128-bit lane 內，
	//    無法解決跨 lane 的搬移需求。
	static void Convert(int8_t* XAMP_RESTRICT output, const int8_t* XAMP_RESTRICT input, const AudioConvertContext& context) noexcept {
		XAMP_EXPECTS(output != nullptr);
		XAMP_EXPECTS(input != nullptr);

		const size_t channels = context.input_format.GetChannels();
		XAMP_EXPECTS(channels == 2); // 此程式碼假設雙聲道 (L,R)

		const size_t convert_size = context.convert_size;

		// 左聲道: 放在 output[0 .. convert_size-1]
	   // 右聲道: 放在 output[convert_size .. (2*convert_size)-1]
		int8_t* left_channel_output = output;
		int8_t* right_channel_output = output + convert_size;

		// 我們一次使用 SSE處理 8 frames = 16 bytes
		// 資料形式: [L0,R0,L1,R1,L2,R2,L3,R3,L4,R4,L5,R5,L6,R6,L7,R7]

		// 準備 shuffle mask
		// left_channel_mask：選取偶數 byte(0,2,4,...) 放入前8 bytes，其餘填 0x80 (會置為0)
		__m128i left_shuffle_mask = _mm_set_epi8(
			(char)0x80, (char)0x80, (char)0x80, (char)0x80,
			(char)0x80, (char)0x80, (char)0x80, (char)0x80,
			14, 12, 10, 8, 6, 4, 2, 0
		);

		// right_channel_mask：選取奇數 byte(1,3,5,...) 同理
		__m128i right_shuffle_mask = _mm_set_epi8(
			(char)0x80, (char)0x80, (char)0x80, (char)0x80,
			(char)0x80, (char)0x80, (char)0x80, (char)0x80,
			15, 13, 11, 9, 7, 5, 3, 1
		);

		size_t i = 0;
		const size_t frames = convert_size;
		const size_t frame_size = 2; // L,R 各1byte

		// 向量化處理 8 frames * 2 channels = 16 bytes
		for (; i + 8 <= frames; i += 8) {
			__m128i input_values = _mm_loadu_si128(reinterpret_cast<const __m128i*>(input));

			__m128i left_values = _mm_shuffle_epi8(input_values, left_shuffle_mask);
			__m128i right_values = _mm_shuffle_epi8(input_values, right_shuffle_mask);

			// left_values, right_values此時低8 bytes存放 8 個樣本 (另一半為0)
			// 直接存入對應指標位置即可
			_mm_storel_epi64(reinterpret_cast<__m128i*>(left_channel_output), left_values);
			_mm_storel_epi64(reinterpret_cast<__m128i*>(right_channel_output), right_values);

			input += frame_size * 8;       // 前進16 bytes
			left_channel_output += 8;      // 左聲道增加8 samples
			right_channel_output += 8;     // 右聲道增加8 samples
		}

		// 尾端不足8 frame 用標量處理
		for (; i < frames; ++i) {
			// interleaved: [L,R]
			// planar: L... R...
			left_channel_output[0] = input[0];
			right_channel_output[0] = input[1];

			input += frame_size;
			left_channel_output++;
			right_channel_output++;
		}
	}

	static void SSEConvert(int32_t* XAMP_RESTRICT output, const int32_t* XAMP_RESTRICT input, const AudioConvertContext& context) noexcept {
		XAMP_EXPECTS(output != nullptr);
		XAMP_EXPECTS(input != nullptr);

		const size_t frames = context.convert_size;

		// Planar 格式分離：左聲道放在 output[0 .. frames-1]
		// 右聲道放在 output[frames .. (2*frames)-1]
		int32_t* left_channel = output;
		int32_t* right_channel = output + frames;

		// 一次處理2 frames = 4 samples (L0,R0,L1,R1) = 16 bytes
		// 使用 _mm_shuffle_epi32 重組 dwords:
		// 輸入: [L0(0),R0(1),L1(2),R1(3)]
		// 左聲道: L0,L1 => result[0]=input[0], result[1]=input[2]
		// 可用 _MM_SHUFFLE(3,3,2,0): result[0]=input[0], result[1]=input[2]
		constexpr int left_mask = _MM_SHUFFLE(3, 3, 2, 0);

		// 右聲道: R0,R1 => result[0]=input[1], result[1]=input[3]
		// 可用 _MM_SHUFFLE(3,3,3,1): result[0]=input[1], result[1]=input[3]
		constexpr int right_mask = _MM_SHUFFLE(3, 3, 3, 1);

		size_t i = 0;
		for (; i + 2 <= frames; i += 2) {
			__m128i input_values = _mm_loadu_si128(reinterpret_cast<const __m128i*>(input));

			__m128i left_values = _mm_shuffle_epi32(input_values, left_mask);
			__m128i right_values = _mm_shuffle_epi32(input_values, right_mask);

			// _mm_storel_epi64儲存低64 bits (即2個int32)
			_mm_storel_epi64(reinterpret_cast<__m128i*>(left_channel), left_values);
			_mm_storel_epi64(reinterpret_cast<__m128i*>(right_channel), right_values);

			input += 4;         // 2 frames * 2ch = 4 int32前進
			left_channel += 2;  // left channel 增加2個樣本
			right_channel += 2; // right channel 增加2個樣本
		}

		// 尾端若剩1 frame (2 samples) 標量處理
		for (; i < frames; ++i) {
			left_channel[0] = input[0];   // L
			right_channel[0] = input[1];  // R
			input += 2;
			left_channel++;
			right_channel++;
		}
	}

	static void Convert(int32_t* XAMP_RESTRICT output, const float* XAMP_RESTRICT input, const AudioConvertContext& context) noexcept {
		XAMP_EXPECTS(output != nullptr);
		XAMP_EXPECTS(input != nullptr);

		const size_t frames = context.convert_size;
		int32_t* left_channel = output;
		int32_t* right_channel = output + frames;

		// 每 frame = 2 floats (L,R)
		// AVX2一次可以處理 8 個 floats = 4 frames
		// Layout: [L0,R0,L1,R1,L2,R2,L3,R3] (index 0=L0,1=R0,2=L1,3=R1,4=L2,5=R2,6=L3,7=R3)
		//
		// 轉成int32後為8個int32:
		// 左聲道 indices: 0,2,4,6
		// 右聲道 indices: 1,3,5,7
		//
		// 利用 _mm256_permutevar8x32_epi32 重組:
		// 左聲道想要順序 [0,2,4,6] = (0,2,4,6)
		// 右聲道想要順序 [1,3,5,7] = (1,3,5,7)
		//
		// _mm256_permutevar8x32_epi32 需要給定一個控制索引的 __m256i
		// ex: for left: index控制向量 = {0,2,4,6, ...其餘可填入任意不取值位置...}
		// 但我們只存前4個int32，因此可用 _mm256_castsi256_si128 取低128-bit後存。
		//
		// 在此範例中，我們取前4個int32即可透過 _mm_storeu_si128 儲存。
		//
		// Left permute mask: {0,2,4,6, ...}
		// Right permute mask: {1,3,5,7, ...}

		const __m256i left_perm_mask = _mm256_setr_epi32(0, 2, 4, 6, 0, 0, 0, 0);
		const __m256i right_perm_mask = _mm256_setr_epi32(1, 3, 5, 7, 0, 0, 0, 0);
		const __m256 scale = _mm256_set1_ps(kFloat32Scale);

		size_t i = 0;
		for (; i + 4 <= frames; i += 4) {
			// 載入8個 floats
			__m256 input_values = _mm256_loadu_ps(input);

			// float->int32
			__m256 scaled_values = _mm256_mul_ps(input_values, scale);
			__m256i int_values = _mm256_cvtps_epi32(scaled_values);

			// 分離左聲道 (0,2,4,6)
			__m256i left_values = _mm256_permutevar8x32_epi32(int_values, left_perm_mask);
			// 分離右聲道 (1,3,5,7)
			__m256i right_values = _mm256_permutevar8x32_epi32(int_values, right_perm_mask);

			// left_values, right_values 各有8個int32，但我們只在乎前4個值
			// 轉回128-bit儲存
			__m128i left_128 = _mm256_castsi256_si128(left_values);
			__m128i right_128 = _mm256_castsi256_si128(right_values);

			// 儲存4個int32
			_mm_storeu_si128(reinterpret_cast<__m128i*>(left_channel), left_128);
			_mm_storeu_si128(reinterpret_cast<__m128i*>(right_channel), right_128);

			input += 8; // 4 frames * 2ch = 8 floats
			left_channel += 4;
			right_channel += 4;
		}

		// 處理不足4 frames的尾端
		for (; i < frames; ++i) {
			float L = input[0];
			float R = input[1];
			// 簡單四捨五入
			int32_t Li = static_cast<int32_t>(L > 0.0f ? L + 0.5f : L - 0.5f);
			int32_t Ri = static_cast<int32_t>(R > 0.0f ? R + 0.5f : R - 0.5f);

			left_channel[0] = Li;
			right_channel[0] = Ri;

			input += 2;
			left_channel++;
			right_channel++;
		}
	}

	static void SSEConvert(int32_t* XAMP_RESTRICT output, const float* XAMP_RESTRICT input, const AudioConvertContext& context) noexcept {
		const size_t frames = context.convert_size;

		// Planar 格式分離：
		// 左聲道: output[0 .. frames-1]
		// 右聲道: output[frames .. 2*frames-1]
		int32_t* left_channel = output;
		int32_t* right_channel = output + frames;

		// 每 frame 有2個 samples (L,R)
		// 一次使用SSE處理2 frames = 4 floats = 16 bytes
		// 載入後資料排列: [L0, R0, L1, R1] (index:0=L0,1=R0,2=L1,3=R1)
		// 對int32順序:
		// 左聲道需 (L0,L1) = index(0,2)
		// 右聲道需 (R0,R1) = index(1,3)
		//
		// 使用 _mm_shuffle_epi32 來重組順序:
		// left_mask: 想取index 0,2，對應 Shuffle為 _MM_SHUFFLE(3,3,2,0)
		// right_mask: 想取index 1,3，對應 Shuffle為 _MM_SHUFFLE(3,3,3,1)

		const int left_mask = _MM_SHUFFLE(3, 3, 2, 0);
		const int right_mask = _MM_SHUFFLE(3, 3, 3, 1);
		const __m128 scale = _mm_set1_ps(kFloat32Scale);

		size_t i = 0;
		for (; i + 2 <= frames; i += 2) {
			// 載入4個 float：L0,R0,L1,R1
			__m128 input_values = _mm_loadu_ps(input);

			// 轉換為 int32
			__m128 scaled_values = _mm_mul_ps(input_values, scale);
			__m128i int_values = _mm_cvtps_epi32(scaled_values);

			// 重組索引以分離左聲道 (0,2) 與右聲道 (1,3)
			__m128i left_values = _mm_shuffle_epi32(int_values, left_mask);
			__m128i right_values = _mm_shuffle_epi32(int_values, right_mask);

			// left_values, right_values: 各自含2個int32
			// 儲存2個int32 (8 bytes)
			_mm_storel_epi64(reinterpret_cast<__m128i*>(left_channel), left_values);
			_mm_storel_epi64(reinterpret_cast<__m128i*>(right_channel), right_values);

			input += 4;            // 前進2 frames * 2ch = 4 floats
			left_channel += 2;     // 左聲道加2 samples
			right_channel += 2;    // 右聲道加2 samples
		}

		// 處理剩餘的frame (若有1 frame剩餘)
		for (; i < frames; ++i) {
			// 取出 L,R
			float L = input[0];
			float R = input[1];
			// 轉為int32
			int32_t Li = static_cast<int32_t>(L > 0.0f ? L + 0.5f : L - 0.5f); // 簡單的四捨五入方式
			int32_t Ri = static_cast<int32_t>(R > 0.0f ? R + 0.5f : R - 0.5f);

			left_channel[0] = Li;
			right_channel[0] = Ri;

			input += 2;
			left_channel++;
			right_channel++;
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
