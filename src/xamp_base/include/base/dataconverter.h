//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
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
	float volume_factor{1.0};
    size_t cache_volume{0};
    size_t convert_size{0};
};

XAMP_BASE_API AudioConvertContext MakeConvert(size_t convert_size) noexcept;

#ifdef XAMP_OS_WIN

XAMP_BASE_API inline void ConvertInt8ToInt8SSE(const int8_t* input, int8_t* left_ptr, int8_t* right_ptr, size_t frames) {
	// mask: 取出「偶數索引」=> [0,2,4,6,8,10,12,14]，其餘填 0x80 (表示不取)
	// mask: 16 bytes
	alignas(16) static constexpr int8_t mask_even[16] = {
		0,2,4,6, 8,10,12,14,  // 依序抓偶數 index
		static_cast<int8_t>(0x80),static_cast<int8_t>(0x80),static_cast<int8_t>(0x80),static_cast<int8_t>(0x80),
		static_cast<int8_t>(0x80),static_cast<int8_t>(0x80),static_cast<int8_t>(0x80),static_cast<int8_t>(0x80)
	};

	// mask: 取出「奇數索引」=> [1,3,5,7,9,11,13,15]
	alignas(16) static constexpr int8_t mask_odd[16] = {
		1,3,5,7, 9,11,13,15,
		static_cast<int8_t>(0x80),static_cast<int8_t>(0x80),static_cast<int8_t>(0x80),static_cast<int8_t>(0x80),
		static_cast<int8_t>(0x80),static_cast<int8_t>(0x80),static_cast<int8_t>(0x80),static_cast<int8_t>(0x80)
	};

	__m128i vMaskEven = _mm_load_si128(reinterpret_cast<const __m128i*>(mask_even));
	__m128i vMaskOdd = _mm_load_si128(reinterpret_cast<const __m128i*>(mask_odd));

	size_t i = 0;

	while (frames >= 8) {
		// 讀取 16 bytes => SSE寄存器
		__m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(input));
		// data=[L0,R0,L1,R1, L2,R2,L3,R3, L4,R4,L5,R5, L6,R6,L7,R7]

		// 取偶數索引 => left
		// _mm_shuffle_epi8( data, vMaskEven )
		// 會把 data中 index=[0,2,4,6,8,10,12,14] 的 byte 抽出到輸出向量的前 8 bytes 
		// 後 8 bytes 若 mask=0x80 => 填0
		__m128i leftVal = _mm_shuffle_epi8(data, vMaskEven);

		// 取奇數索引 => right
		__m128i rightVal = _mm_shuffle_epi8(data, vMaskOdd);

		// 只需要前 8 bytes => [L0..L7] / [R0..R7]
		// 可用 _mm_storel_epi64 寫 8 bytes
		_mm_storel_epi64(reinterpret_cast<__m128i*>(left_ptr), leftVal);
		_mm_storel_epi64(reinterpret_cast<__m128i*>(right_ptr), rightVal);

		// 更新指標
		input += 16;  // 8 frames => 16 bytes
		left_ptr += 8;
		right_ptr += 8;
		frames -= 8;
	}

	// leftover 標量: frames 個 frame => 2*frames bytes
	for (; frames > 0; frames--) {
		*left_ptr++ = *input++;  // L
		*right_ptr++ = *input++;  // R
	}
}

XAMP_BASE_API inline void ConvertFloatToFloatSSE(const float* input, float* left_ptr, float* right_ptr, size_t frames) {
	size_t i = 0;

	for (; i + 4 <= frames; i += 4) {
		// 讀入 8 個 floats
		__m128 in1 = _mm_loadu_ps(input);       // [L0,R0,L1,R1]
		__m128 in2 = _mm_loadu_ps(input + 4);   // [L2,R2,L3,R3]

		// 第 1 級 unpack: 把 in1, in2 拆成 low, high
		//   low  = [L0,L2, R0,R2]
		//   high = [L1,L3, R1,R3]
		__m128 low = _mm_unpacklo_ps(in1, in2);
		__m128 high = _mm_unpackhi_ps(in1, in2);

		// 第 2 級 unpack: 
		//   left  = [L0,L1,L2,L3]
		//   right = [R0,R1,R2,R3]
		__m128 left = _mm_unpacklo_ps(low, high);
		__m128 right = _mm_unpackhi_ps(low, high);

		// 寫回 planar
		_mm_storeu_ps(left_ptr, left);     // 寫出 (L0,L1,L2,L3)
		_mm_storeu_ps(right_ptr, right);   // 寫出 (R0,R1,R2,R3)

		left_ptr += 4;
		right_ptr += 4;
		input += 8;  // 處理了 4 frames => 8 floats
	}

	// 尾端不足4 frames 用標量處理
	for (; i < frames; i++) {
		left_ptr[0] = input[0];  // L
		right_ptr[0] = input[1];  // R
		left_ptr++;
		right_ptr++;
		input += 2;
	}
}

XAMP_BASE_API inline void ConvertFloatToInt16SSE(const float* input, int16_t* left_ptr, int16_t* right_ptr, size_t frames) {
	// 一次處理 4 frames => 8 個 float： [L0,R0, L1,R1, L2,R2, L3,R3]
   // SSE 一次可載入 4 個 float => in1=[L0,R0,L1,R1], in2=[L2,R2,L3,R3]
   // 再透過兩級 unpacklo/hi_ps 直接分離出 left=[L0,L1,L2,L3], right=[R0,R1,R2,R3]

	size_t i = 0;
	__m128 scale = _mm_set1_ps(kFloat16Scale);

	for (; i + 4 <= frames; i += 4) {
		// 1) 載入 8 個 floats
		__m128 in1 = _mm_loadu_ps(input);       // => L0,R0,L1,R1
		__m128 in2 = _mm_loadu_ps(input + 4);   // => L2,R2,L3,R3

		// 2) 乘以縮放常數 => float => ±32767
		in1 = _mm_mul_ps(in1, scale);
		in2 = _mm_mul_ps(in2, scale);

		// 3) 第一級 unpack => 
		//    low  = [L0,L2, R0,R2]
		//    high = [L1,L3, R1,R3]
		__m128 low = _mm_unpacklo_ps(in1, in2);
		__m128 high = _mm_unpackhi_ps(in1, in2);

		// 4) 第二級 unpack 分離左、右聲道
		//    left  = [L0,L1, L2,L3]
		//    right = [R0,R1, R2,R3]
		__m128 left = _mm_unpacklo_ps(low, high);
		__m128 right = _mm_unpackhi_ps(low, high);

		// 5) float => int32 => packs => int16
		__m128i left_i32 = _mm_cvttps_epi32(left);
		__m128i right_i32 = _mm_cvttps_epi32(right);

		// _mm_packs_epi32: 
		//   前4個 int16(低64bits) = left_i32，
		//   後4個 int16(高64bits) = right_i32。
		// 形成 packed: [L0,L1,L2,L3,  R0,R1,R2,R3](int16)
		__m128i packed = _mm_packs_epi32(left_i32, right_i32);

		// 6) 把前4個 int16 存到 left_ptr、後4個 int16 存到 right_ptr
		//    這裡可以先用 _mm_storel_epi64 拿低64bits給 left，再用 hi 取高64bits給 right
		_mm_storel_epi64(reinterpret_cast<__m128i*>(left_ptr), packed);
		__m128i hi = _mm_unpackhi_epi64(packed, packed);
		_mm_storel_epi64(reinterpret_cast<__m128i*>(right_ptr), hi);

		// 7) 更新指標
		left_ptr += 4;
		right_ptr += 4;
		input += 8;  // 已處理 4 frames => 8 floats
	}

	// 8) 尾端不足4 frames => 標量處理
	for (; i < frames; i++) {
		float L = input[0] * kFloat16Scale;
		float R = input[1] * kFloat16Scale;
		// cast => int16 (截斷, 若要四捨五入可加 0.5f)
		int16_t Li = static_cast<int16_t>(L);
		int16_t Ri = static_cast<int16_t>(R);
		*left_ptr++ = Li;
		*right_ptr++ = Ri;
		input += 2;
	}
}

XAMP_BASE_API inline void ConvertFloatToInt24(const float* input, int32_t* left_ptr, int32_t* right_ptr, size_t frames) {
	for (size_t i = 0; i < frames; i++) {
		// interleaved float: L,R
		float L = input[2 * i + 0] * kFloat24Scale;
		float R = input[2 * i + 1] * kFloat24Scale;

		// 轉成 int32 後 << 8 以對齊 24 bit
		int32_t Li = static_cast<int32_t>(L) << 8;
		int32_t Ri = static_cast<int32_t>(R) << 8;

		// planar 輸出: left全部存在 left_ptr，right全部存在 right_ptr
		left_ptr[i] = Li;
		right_ptr[i] = Ri;
	}
}

XAMP_BASE_API inline void ConvertFloatToInt16(const float* input, int16_t* left_ptr, int16_t* right_ptr, size_t frames) {
	for (size_t i = 0; i < frames; i++) {
		// interleaved float: L,R
		float L = input[2 * i + 0] * kFloat16Scale;
		float R = input[2 * i + 1] * kFloat16Scale;

		// 轉成 int16
		int16_t Li = static_cast<int16_t>(L);
		int16_t Ri = static_cast<int16_t>(R);

		// planar 輸出: left全部存在 left_ptr，right全部存在 right_ptr
		left_ptr[i] = Li;
		right_ptr[i] = Ri;
	}
}

XAMP_BASE_API inline void ConvertFloatToInt32SSE(const float* input, int32_t* left_ptr, int32_t* right_ptr, size_t frames, float volume = 1.0) {
	size_t i = 0;
	__m128 scale = _mm_set1_ps(kFloat32Scale);
	__m128 volume_scale = _mm_set1_ps(volume);

	for (; i + 4 <= frames; i += 4) {
		__m128 in1 = _mm_loadu_ps(input);     // L0,R0,L1,R1
		__m128 in2 = _mm_loadu_ps(input + 4); // L2,R2,L3,R3

		// 乘以縮放因子
		in1 = _mm_mul_ps(in1, scale);
		in2 = _mm_mul_ps(in2, scale);

		in1 = _mm_mul_ps(in1, volume_scale);
		in2 = _mm_mul_ps(in2, volume_scale);

		// 使用 unpacklo/hi 直接分離左右聲道
		__m128 low = _mm_unpacklo_ps(in1, in2);   // L0,L2,R0,R2
		__m128 high = _mm_unpackhi_ps(in1, in2);  // L1,L3,R1,R3

		// 再次 unpack 得到最終順序
		__m128 left = _mm_unpacklo_ps(low, high);  // L0,L1,L2,L3
		__m128 right = _mm_unpackhi_ps(low, high); // R0,R1,R2,R3

		// 轉換並存儲
		__m128i left_i32 = _mm_cvttps_epi32(left);
		__m128i right_i32 = _mm_cvttps_epi32(right);

		_mm_storeu_si128(reinterpret_cast<__m128i*>(left_ptr), left_i32);
		_mm_storeu_si128(reinterpret_cast<__m128i*>(right_ptr), right_i32);

		left_ptr += 4;
		right_ptr += 4;
		input += 8;
	}

	// 尾端不足4 frames標量處理
	for (; i < frames; i++) {
		float L = input[0] * kFloat32Scale;
		float R = input[1] * kFloat32Scale;
		int32_t Li = static_cast<int32_t>(L);
		int32_t Ri = static_cast<int32_t>(R);
		*left_ptr++ = Li;
		*right_ptr++ = Ri;
		input += 2;
	}
}

XAMP_BASE_API inline void ConvertFloatToInt24SSE(const float* input, int32_t* left_ptr, int32_t* right_ptr, size_t frames) {
	size_t i = 0;
	__m128 scale = _mm_set1_ps(kFloat24Scale);

	for (; i + 4 <= frames; i += 4) {
		__m128 in1 = _mm_loadu_ps(input);     // L0,R0,L1,R1
		__m128 in2 = _mm_loadu_ps(input + 4); // L2,R2,L3,R3

		// 乘以縮放因子
		in1 = _mm_mul_ps(in1, scale);
		in2 = _mm_mul_ps(in2, scale);

		// 使用 unpacklo/hi 直接分離左右聲道
		__m128 low = _mm_unpacklo_ps(in1, in2);   // L0,L2,R0,R2
		__m128 high = _mm_unpackhi_ps(in1, in2);  // L1,L3,R1,R3

		// 再次 unpack 得到最終順序
		__m128 left = _mm_unpacklo_ps(low, high);  // L0,L1,L2,L3
		__m128 right = _mm_unpackhi_ps(low, high); // R0,R1,R2,R3

		// 轉換並存儲
		__m128i left_i32 = _mm_cvttps_epi32(left);
		__m128i right_i32 = _mm_cvttps_epi32(right);

		_mm_storeu_si128(reinterpret_cast<__m128i*>(left_ptr), _mm_slli_epi32(left_i32, 8));
		_mm_storeu_si128(reinterpret_cast<__m128i*>(right_ptr), _mm_slli_epi32(right_i32, 8));

		left_ptr += 4;
		right_ptr += 4;
		input += 8;
	}

	// 尾端不足4 frames標量處理
	for (; i < frames; i++) {
		float L = input[0] * kFloat24Scale;
		float R = input[1] * kFloat24Scale;
		int32_t Li = static_cast<int32_t>(L) << 8;
		int32_t Ri = static_cast<int32_t>(R) << 8;
		*left_ptr++ = Li;
		*right_ptr++ = Ri;
		input += 2;
	}
}

template <typename T, typename TStoreType = T>
void AVX2Convert(TStoreType* output, const float* input, float float_scale, const AudioConvertContext& context) noexcept {
	const auto* end_input = input + static_cast<ptrdiff_t>(context.convert_size) * AudioFormat::kMaxChannel;

	const __m256 scale = _mm256_set1_ps(float_scale);
	const __m256 volume_scale = _mm256_set1_ps(context.volume_factor);

	while (input + 8 <= end_input) {
		XAMP_ASSERT(end_input - input > 0);

		__m256 input_values = _mm256_load_ps(input);

		__m256 scaled_values = _mm256_mul_ps(input_values, scale);
		scaled_values = _mm256_mul_ps(scaled_values, volume_scale);

		if constexpr (std::is_same_v<T, int32_t>) {
			__m256i output_values = _mm256_cvttps_epi32(scaled_values);
			_mm256_store_si256(reinterpret_cast<__m256i*>(output), output_values);
			output += 8;
		}
		else if constexpr (sizeof(T) == 3) {
			__m256i output_values = _mm256_cvtps_epi32(scaled_values);
			alignas(32) int32_t temp_output[8];
			_mm256_store_si256(reinterpret_cast<__m256i*>(temp_output), output_values);
			__m256i temp_output_values = _mm256_load_si256(reinterpret_cast<const __m256i*>(temp_output));
			__m256i shifted_values = _mm256_slli_si256(temp_output_values, 1);
			_mm256_store_si256(reinterpret_cast<__m256i*>(output), shifted_values);
			output += 8;
		}
		else if constexpr (std::is_same_v<T, int16_t>) {
			__m256i output_values = _mm256_cvttps_epi32(scaled_values);
			__m256i packed_values = _mm256_packs_epi32(output_values, output_values);
			packed_values = _mm256_permute4x64_epi64(packed_values, 0xD8);
			_mm_store_si128(reinterpret_cast<__m128i*>(output), _mm256_castsi256_si128(packed_values));
			output += 8;
		}
		else {
			__m256 output_values = scaled_values;
			_mm256_store_ps(reinterpret_cast<float*>(output), output_values);
			output += 8;
		}

		input += 8;
	}

	while (input != end_input) {
		XAMP_ASSERT(end_input - input > 0);
		if constexpr (sizeof(T) == 3) {
			int32_t temp = static_cast<int32_t>(*input * float_scale) << 8;
			*reinterpret_cast<int32_t*>(output) = temp;
			output += 1;
		}
		else if constexpr (std::is_same_v<T, int16_t>) {
			*output = static_cast<int16_t>(*input * float_scale);
			++output;
		}
		else {
			*output = static_cast<T>(*input * float_scale);
			++output;
		}
		++input;
	}
}

template <PackedFormat InputFormat, PackedFormat OutputFormat>
struct XAMP_BASE_API_ONLY_EXPORT DataConverter {};

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
	static void Convert(int8_t* output, const int8_t* input, const AudioConvertContext& context) noexcept {
		const size_t convert_size = context.convert_size;
		// 左聲道: 放在 output[0 .. convert_size-1]
		// 右聲道: 放在 output[convert_size .. (2*convert_size)-1]
		int8_t* left_channel_output = output;
		int8_t* right_channel_output = output + convert_size;

		// 我們一次使用 SSE處理 8 frames = 16 bytes
		// 資料形式: [L0,R0,L1,R1,L2,R2,L3,R3,L4,R4,L5,R5,L6,R6,L7,R7]

		// 準備 shuffle mask
		// left_channel_mask：選取偶數 byte(0,2,4,...) 放入前8 bytes，其餘填 0x80 (會置為0)
		static const __m128i left_shuffle_mask = _mm_set_epi8(
			static_cast<char>(0x80), static_cast<char>(0x80), static_cast<char>(0x80), static_cast<char>(0x80),
			static_cast<char>(0x80), static_cast<char>(0x80), static_cast<char>(0x80), static_cast<char>(0x80),
			14, 12, 10, 8, 6, 4, 2, 0
		);

		// right_channel_mask：選取奇數 byte(1,3,5,...) 同理
		static const __m128i right_shuffle_mask = _mm_set_epi8(
			static_cast<char>(0x80), static_cast<char>(0x80), static_cast<char>(0x80), static_cast<char>(0x80),
			static_cast<char>(0x80), static_cast<char>(0x80), static_cast<char>(0x80), static_cast<char>(0x80),
			15, 13, 11, 9, 7, 5, 3, 1
		);

		size_t i = 0;
		const size_t frames = convert_size;
		constexpr size_t frame_size = 2; // L,R 各1byte

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

	static void Convert(int32_t* output, const float* input, const AudioConvertContext& context) noexcept {
		const size_t frames = context.convert_size;
		int32_t* left_channel = output;
		int32_t* right_channel = output + frames;
		ConvertFloatToInt32SSE(input, left_channel, right_channel, frames);
	}
};

template <>
struct XAMP_BASE_API_ONLY_EXPORT DataConverter<PackedFormat::INTERLEAVED, PackedFormat::INTERLEAVED> {
	static void Convert(int16_t* output, const float* input, const AudioConvertContext& context) noexcept {
		AVX2Convert<int16_t>(output, input, kFloat16Scale, context);
	}

	static void Convert(int32_t* output, const float* input, const AudioConvertContext& context) noexcept {
		AVX2Convert<int32_t>(output, input, kFloat32Scale, context);
	}

	static void ConvertToInt2432(int32_t* output, const float* input, const AudioConvertContext& context) noexcept {
		AVX2Convert<Int24, int32_t>(output, input, kFloat24Scale, context);
	}
};
#endif

XAMP_BASE_NAMESPACE_END
