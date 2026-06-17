#include <base/dataconverter.h>
#include <base/assert.h>

#include <span>
#include <type_traits>

#if defined(XAMP_OS_WIN) || defined(__AVX2__)
#define XAMP_DATACONVERTER_HAS_X86_SIMD 1
#include <immintrin.h>
#else
#define XAMP_DATACONVERTER_HAS_X86_SIMD 0
#endif

XAMP_BASE_NAMESPACE_BEGIN

#if XAMP_DATACONVERTER_HAS_X86_SIMD

void convertInt8ToInt8SSE(const int8_t* input, int8_t* left_ptr, int8_t* right_ptr, size_t frames) {
	XAMP_ASSUME(input != nullptr);
	XAMP_ASSUME(left_ptr != nullptr);
	XAMP_ASSUME(right_ptr != nullptr);

	// Deinterleave 8 stereo int8 frames: even bytes are left, odd bytes are right.
	alignas(16) static constexpr int8_t mask_even[16] = {
		0,2,4,6, 8,10,12,14,
		static_cast<int8_t>(0x80),static_cast<int8_t>(0x80),static_cast<int8_t>(0x80),static_cast<int8_t>(0x80),
		static_cast<int8_t>(0x80),static_cast<int8_t>(0x80),static_cast<int8_t>(0x80),static_cast<int8_t>(0x80)
	};

	alignas(16) static constexpr int8_t mask_odd[16] = {
		1,3,5,7, 9,11,13,15,
		static_cast<int8_t>(0x80),static_cast<int8_t>(0x80),static_cast<int8_t>(0x80),static_cast<int8_t>(0x80),
		static_cast<int8_t>(0x80),static_cast<int8_t>(0x80),static_cast<int8_t>(0x80),static_cast<int8_t>(0x80)
	};

	__m128i vMaskEven = _mm_load_si128(reinterpret_cast<const __m128i*>(mask_even));
	__m128i vMaskOdd = _mm_load_si128(reinterpret_cast<const __m128i*>(mask_odd));

	size_t i = 0;

	while (frames >= 8) {
		__m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(input));
		__m128i leftVal = _mm_shuffle_epi8(data, vMaskEven);
		__m128i rightVal = _mm_shuffle_epi8(data, vMaskOdd);

		_mm_storel_epi64(reinterpret_cast<__m128i*>(left_ptr), leftVal);
		_mm_storel_epi64(reinterpret_cast<__m128i*>(right_ptr), rightVal);

		input += 16;
		left_ptr += 8;
		right_ptr += 8;
		frames -= 8;
	}

	for (; frames > 0; frames--) {
		*left_ptr++ = *input++;
		*right_ptr++ = *input++;
	}
}

void convertFloatToFloatSSE(const float* input, float* left_ptr, float* right_ptr, size_t frames) {
	XAMP_ASSUME(input != nullptr);
	XAMP_ASSUME(left_ptr != nullptr);
	XAMP_ASSUME(right_ptr != nullptr);

	size_t i = 0;

	for (; i + 4 <= frames; i += 4) {
		__m128 in1 = _mm_loadu_ps(input);
		__m128 in2 = _mm_loadu_ps(input + 4);

		// Two unpack stages convert [L0,R0,L1,R1,L2,R2,L3,R3] into planar lanes.
		__m128 low = _mm_unpacklo_ps(in1, in2);
		__m128 high = _mm_unpackhi_ps(in1, in2);

		__m128 left = _mm_unpacklo_ps(low, high);
		__m128 right = _mm_unpackhi_ps(low, high);

		_mm_storeu_ps(left_ptr, left);
		_mm_storeu_ps(right_ptr, right);

		left_ptr += 4;
		right_ptr += 4;
		input += 8;
	}

	for (; i < frames; i++) {
		left_ptr[0] = input[0];
		right_ptr[0] = input[1];
		left_ptr++;
		right_ptr++;
		input += 2;
	}
}

void convertFloatToInt16SSE(const float* input, int16_t* left_ptr, int16_t* right_ptr, size_t frames) {
	XAMP_ASSUME(input != nullptr);
	XAMP_ASSUME(left_ptr != nullptr);
	XAMP_ASSUME(right_ptr != nullptr);

	size_t i = 0;
	__m128 scale = _mm_set1_ps(kFloat16Scale);

	for (; i + 4 <= frames; i += 4) {
		__m128 in1 = _mm_loadu_ps(input);
		__m128 in2 = _mm_loadu_ps(input + 4);

		in1 = _mm_mul_ps(in1, scale);
		in2 = _mm_mul_ps(in2, scale);

		// convert 4 interleaved stereo frames to two planar vectors before packing to int16.
		__m128 low = _mm_unpacklo_ps(in1, in2);
		__m128 high = _mm_unpackhi_ps(in1, in2);

		__m128 left = _mm_unpacklo_ps(low, high);
		__m128 right = _mm_unpackhi_ps(low, high);

		__m128i left_i32 = _mm_cvttps_epi32(left);
		__m128i right_i32 = _mm_cvttps_epi32(right);

		__m128i packed = _mm_packs_epi32(left_i32, right_i32);

		_mm_storel_epi64(reinterpret_cast<__m128i*>(left_ptr), packed);
		__m128i hi = _mm_unpackhi_epi64(packed, packed);
		_mm_storel_epi64(reinterpret_cast<__m128i*>(right_ptr), hi);

		left_ptr += 4;
		right_ptr += 4;
		input += 8;
	}

	for (; i < frames; i++) {
		float L = input[0] * kFloat16Scale;
		float R = input[1] * kFloat16Scale;
		int16_t Li = static_cast<int16_t>(L);
		int16_t Ri = static_cast<int16_t>(R);
		*left_ptr++ = Li;
		*right_ptr++ = Ri;
		input += 2;
	}
}

void convertFloatToInt24(const float* input, int32_t* left_ptr, int32_t* right_ptr, size_t frames) {
	XAMP_ASSUME(input != nullptr);
	XAMP_ASSUME(left_ptr != nullptr);
	XAMP_ASSUME(right_ptr != nullptr);

	for (size_t i = 0; i < frames; i++) {
		float L = input[2 * i + 0] * kFloat24Scale;
		float R = input[2 * i + 1] * kFloat24Scale;

		// 24-bit samples are left-aligned in the 32-bit container.
		int32_t Li = static_cast<int32_t>(L) << 8;
		int32_t Ri = static_cast<int32_t>(R) << 8;

		left_ptr[i] = Li;
		right_ptr[i] = Ri;
	}
}

void convertFloatToInt16(const float* input, int16_t* left_ptr, int16_t* right_ptr, size_t frames) {
	XAMP_ASSUME(input != nullptr);
	XAMP_ASSUME(left_ptr != nullptr);
	XAMP_ASSUME(right_ptr != nullptr);

	for (size_t i = 0; i < frames; i++) {
		float L = input[2 * i + 0] * kFloat16Scale;
		float R = input[2 * i + 1] * kFloat16Scale;

		int16_t Li = static_cast<int16_t>(L);
		int16_t Ri = static_cast<int16_t>(R);

		left_ptr[i] = Li;
		right_ptr[i] = Ri;
	}
}

void convertFloatToInt32SSE(const float* input, int32_t* left_ptr, int32_t* right_ptr, size_t frames, float volume) {
	XAMP_ASSUME(input != nullptr);
	XAMP_ASSUME(left_ptr != nullptr);
	XAMP_ASSUME(right_ptr != nullptr);

	size_t i = 0;
	__m128 scale = _mm_set1_ps(kFloat32Scale);
	__m128 volume_scale = _mm_set1_ps(volume);

	for (; i + 4 <= frames; i += 4) {
		__m128 in1 = _mm_loadu_ps(input);
		__m128 in2 = _mm_loadu_ps(input + 4);

		in1 = _mm_mul_ps(in1, scale);
		in2 = _mm_mul_ps(in2, scale);

		in1 = _mm_mul_ps(in1, volume_scale);
		in2 = _mm_mul_ps(in2, volume_scale);

		__m128 low = _mm_unpacklo_ps(in1, in2);
		__m128 high = _mm_unpackhi_ps(in1, in2);

		__m128 left = _mm_unpacklo_ps(low, high);
		__m128 right = _mm_unpackhi_ps(low, high);

		__m128i left_i32 = _mm_cvttps_epi32(left);
		__m128i right_i32 = _mm_cvttps_epi32(right);

		_mm_storeu_si128(reinterpret_cast<__m128i*>(left_ptr), left_i32);
		_mm_storeu_si128(reinterpret_cast<__m128i*>(right_ptr), right_i32);

		left_ptr += 4;
		right_ptr += 4;
		input += 8;
	}

	for (; i < frames; i++) {
		float L = input[0] * kFloat32Scale * volume;
		float R = input[1] * kFloat32Scale * volume;
		int32_t Li = static_cast<int32_t>(L);
		int32_t Ri = static_cast<int32_t>(R);
		*left_ptr++ = Li;
		*right_ptr++ = Ri;
		input += 2;
	}
}

void convertFloatToInt24SSE(const float* input, int32_t* left_ptr, int32_t* right_ptr, size_t frames) {
	XAMP_ASSUME(input != nullptr);
	XAMP_ASSUME(left_ptr != nullptr);
	XAMP_ASSUME(right_ptr != nullptr);

	size_t i = 0;
	__m128 scale = _mm_set1_ps(kFloat24Scale);

	for (; i + 4 <= frames; i += 4) {
		__m128 in1 = _mm_loadu_ps(input);
		__m128 in2 = _mm_loadu_ps(input + 4);

		in1 = _mm_mul_ps(in1, scale);
		in2 = _mm_mul_ps(in2, scale);

		__m128 low = _mm_unpacklo_ps(in1, in2);
		__m128 high = _mm_unpackhi_ps(in1, in2);

		__m128 left = _mm_unpacklo_ps(low, high);
		__m128 right = _mm_unpackhi_ps(low, high);

		__m128i left_i32 = _mm_cvttps_epi32(left);
		__m128i right_i32 = _mm_cvttps_epi32(right);

		_mm_storeu_si128(reinterpret_cast<__m128i*>(left_ptr), _mm_slli_epi32(left_i32, 8));
		_mm_storeu_si128(reinterpret_cast<__m128i*>(right_ptr), _mm_slli_epi32(right_i32, 8));

		left_ptr += 4;
		right_ptr += 4;
		input += 8;
	}

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

template <typename t, typename TStoreType = t>
void AVX2Convert(TStoreType* output, const float* input, float float_scale, const AudioConvertContext& context) {
	XAMP_ASSUME(input != nullptr);
	XAMP_ASSUME(output != nullptr);

	auto input_samples = std::span{ input, context.convert_size * AudioFormat::kMaxChannel };
	const auto* end_input = input_samples.data() + input_samples.size();

	const __m256 scale = _mm256_set1_ps(float_scale);
	const __m256 volume_scale = _mm256_set1_ps(context.volume_factor);

	while (input + 8 <= end_input) {
		XAMP_ASSERT(end_input - input > 0);

		__m256 input_values = _mm256_loadu_ps(input);

		__m256 scaled_values = _mm256_mul_ps(input_values, scale);
		scaled_values = _mm256_mul_ps(scaled_values, volume_scale);

		if constexpr (std::is_same_v<t, int32_t>) {
			__m256i output_values = _mm256_cvtps_epi32(scaled_values);
			_mm256_storeu_si256(reinterpret_cast<__m256i*>(output), output_values);
			output += 8;
		}
		else if constexpr (sizeof(t) == 3) {
			// WASAPI 24-in-32 output keeps the 24 significant bits left-aligned.
			__m256i output_values = _mm256_cvtps_epi32(scaled_values);
			alignas(32) int32_t temp_output[8];
			_mm256_store_si256(reinterpret_cast<__m256i*>(temp_output), output_values);
			__m256i temp_output_values = _mm256_load_si256(reinterpret_cast<const __m256i*>(temp_output));
			__m256i shifted_values = _mm256_slli_si256(temp_output_values, 1);
			_mm256_storeu_si256(reinterpret_cast<__m256i*>(output), shifted_values);
			output += 8;
		}
		else if constexpr (std::is_same_v<t, int16_t>) {
			__m256i output_values = _mm256_cvtps_epi32(scaled_values);
			__m256i packed_values = _mm256_packs_epi32(output_values, output_values);
			packed_values = _mm256_permute4x64_epi64(packed_values, 0xD8);
			_mm_storeu_si128(reinterpret_cast<__m128i*>(output), _mm256_castsi256_si128(packed_values));
			output += 8;
		}
		else {
			__m256 output_values = scaled_values;
			_mm256_storeu_ps(reinterpret_cast<float*>(output), output_values);
			output += 8;
		}

		input += 8;
	}

	while (input != end_input) {
		XAMP_ASSERT(end_input - input > 0);
		const auto scaled_value = *input * float_scale * context.volume_factor;
		if constexpr (sizeof(t) == 3) {
			int32_t temp = static_cast<int32_t>(scaled_value) << 8;
			*reinterpret_cast<int32_t*>(output) = temp;
			output += 1;
		}
		else if constexpr (std::is_same_v<t, int16_t>) {
			*output = static_cast<int16_t>(scaled_value);
			++output;
		}
		else {
			*output = static_cast<t>(scaled_value);
			++output;
		}
		++input;
	}
}

void DataConverter<PackedFormat::INTERLEAVED, PackedFormat::PLANAR>::convert(int8_t* output, const int8_t* input, const AudioConvertContext& context) {
		XAMP_ASSUME(input != nullptr);
		XAMP_ASSUME(output != nullptr);

		auto output_samples = std::span{ output, context.convert_size * AudioFormat::kMaxChannel };
		auto left_channel_output = output_samples.data();
		auto right_channel_output = output_samples.data() + context.convert_size;

		// int8 立體聲拆聲道屬於 byte-level 重組；AVX2 dword permute 無法挑單一 byte，
		// _mm256_shuffle_epi8 又受限於各自的 128-bit lane，因此這裡保留 128-bit shuffle mask。
		static const __m128i left_shuffle_mask = _mm_set_epi8(
			static_cast<char>(0x80), static_cast<char>(0x80), static_cast<char>(0x80), static_cast<char>(0x80),
			static_cast<char>(0x80), static_cast<char>(0x80), static_cast<char>(0x80), static_cast<char>(0x80),
			14, 12, 10, 8, 6, 4, 2, 0
		);

		static const __m128i right_shuffle_mask = _mm_set_epi8(
			static_cast<char>(0x80), static_cast<char>(0x80), static_cast<char>(0x80), static_cast<char>(0x80),
			static_cast<char>(0x80), static_cast<char>(0x80), static_cast<char>(0x80), static_cast<char>(0x80),
			15, 13, 11, 9, 7, 5, 3, 1
		);

		size_t i = 0;
		const size_t frames = context.convert_size;
		constexpr size_t frame_size = 2;

		for (; i + 8 <= frames; i += 8) {
			__m128i input_values = _mm_loadu_si128(reinterpret_cast<const __m128i*>(input));

			__m128i left_values = _mm_shuffle_epi8(input_values, left_shuffle_mask);
			__m128i right_values = _mm_shuffle_epi8(input_values, right_shuffle_mask);

			_mm_storel_epi64(reinterpret_cast<__m128i*>(left_channel_output), left_values);
			_mm_storel_epi64(reinterpret_cast<__m128i*>(right_channel_output), right_values);

			input += frame_size * 8;
			left_channel_output += 8;
			right_channel_output += 8;
		}

		for (; i < frames; ++i) {
			left_channel_output[0] = input[0];
			right_channel_output[0] = input[1];

			input += frame_size;
			left_channel_output++;
			right_channel_output++;
		}
}

void DataConverter<PackedFormat::INTERLEAVED, PackedFormat::PLANAR>::convert(int32_t* output, const float* input, const AudioConvertContext& context) {
	const size_t frames = context.convert_size;
	auto output_samples = std::span{ output, frames * AudioFormat::kMaxChannel };
	convertFloatToInt32SSE(input, output_samples.data(), output_samples.data() + frames, frames);
}

void DataConverter<PackedFormat::INTERLEAVED, PackedFormat::INTERLEAVED>::convert(int16_t* output, const float* input, const AudioConvertContext& context) {
	AVX2Convert<int16_t>(output, input, kFloat16Scale, context);
}

void DataConverter<PackedFormat::INTERLEAVED, PackedFormat::INTERLEAVED>::convertToInt24(int24_t* output, const int32_t* input, const AudioConvertContext& context) {
	// Store the most significant 24 bits from the 32-bit sample container.
	for (size_t i = 0; i < context.convert_size * 2; ++i) {
		output[i] = input[i] >> 8;
	}
}

void DataConverter<PackedFormat::INTERLEAVED, PackedFormat::INTERLEAVED>::convertToInt32(int32_t* output, const float* input, const AudioConvertContext& context) {
	AVX2Convert<int32_t>(output, input, kFloat32Scale, context);
}

void DataConverter<PackedFormat::INTERLEAVED, PackedFormat::INTERLEAVED>::convertToInt2432(int32_t* output, const float* input, const AudioConvertContext& context) {
	AVX2Convert<int24_t, int32_t>(output, input, kFloat24Scale, context);
}

#else

// 未啟用 AVX2 的目標使用純量 fallback，例如 arm64 macOS 或未開 -mavx2 的 Linux。

void convertInt8ToInt8SSE(const int8_t* input, int8_t* left_ptr, int8_t* right_ptr, size_t frames) {
	XAMP_ASSUME(input != nullptr);
	XAMP_ASSUME(left_ptr != nullptr);
	XAMP_ASSUME(right_ptr != nullptr);

	for (size_t i = 0; i < frames; ++i) {
		*left_ptr++ = *input++;
		*right_ptr++ = *input++;
	}
}

void convertFloatToFloatSSE(const float* input, float* left_ptr, float* right_ptr, size_t frames) {
	XAMP_ASSUME(input != nullptr);
	XAMP_ASSUME(left_ptr != nullptr);
	XAMP_ASSUME(right_ptr != nullptr);

	for (size_t i = 0; i < frames; ++i) {
		*left_ptr++ = *input++;
		*right_ptr++ = *input++;
	}
}

void convertFloatToInt16SSE(const float* input, int16_t* left_ptr, int16_t* right_ptr, size_t frames) {
	XAMP_ASSUME(input != nullptr);
	XAMP_ASSUME(left_ptr != nullptr);
	XAMP_ASSUME(right_ptr != nullptr);

	for (size_t i = 0; i < frames; ++i) {
		*left_ptr++ = static_cast<int16_t>(*input++ * kFloat16Scale);
		*right_ptr++ = static_cast<int16_t>(*input++ * kFloat16Scale);
	}
}

void convertFloatToInt32SSE(const float* input, int32_t* left_ptr, int32_t* right_ptr, size_t frames, float volume) {
	XAMP_ASSUME(input != nullptr);
	XAMP_ASSUME(left_ptr != nullptr);
	XAMP_ASSUME(right_ptr != nullptr);

	for (size_t i = 0; i < frames; ++i) {
		*left_ptr++ = static_cast<int32_t>(*input++ * kFloat32Scale * volume);
		*right_ptr++ = static_cast<int32_t>(*input++ * kFloat32Scale * volume);
	}
}

void convertFloatToInt24SSE(const float* input, int32_t* left_ptr, int32_t* right_ptr, size_t frames) {
	XAMP_ASSUME(input != nullptr);
	XAMP_ASSUME(left_ptr != nullptr);
	XAMP_ASSUME(right_ptr != nullptr);

	for (size_t i = 0; i < frames; ++i) {
		*left_ptr++ = static_cast<int32_t>(*input++ * kFloat24Scale) << 8;
		*right_ptr++ = static_cast<int32_t>(*input++ * kFloat24Scale) << 8;
	}
}

void DataConverter<PackedFormat::INTERLEAVED, PackedFormat::PLANAR>::convert(int8_t* output, const int8_t* input, const AudioConvertContext& context) {
	XAMP_ASSUME(input != nullptr);
	XAMP_ASSUME(output != nullptr);

	auto output_samples = std::span{ output, context.convert_size * AudioFormat::kMaxChannel };
	auto* left_channel = output_samples.data();
	auto* right_channel = output_samples.data() + context.convert_size;
	for (size_t i = 0; i < context.convert_size; ++i) {
		*left_channel++ = *input++;
		*right_channel++ = *input++;
	}
}

void DataConverter<PackedFormat::INTERLEAVED, PackedFormat::PLANAR>::convert(int32_t* output, const float* input, const AudioConvertContext& context) {
	XAMP_ASSUME(input != nullptr);
	XAMP_ASSUME(output != nullptr);

	auto input_samples = std::span{ input, context.convert_size * AudioFormat::kMaxChannel };
	auto output_samples = std::span{ output, context.convert_size * AudioFormat::kMaxChannel };
	auto* left_channel = output_samples.data();
	auto* right_channel = output_samples.data() + context.convert_size;
	for (size_t i = 0; i < context.convert_size; ++i) {
		*left_channel++ = static_cast<int32_t>(input_samples[i * 2] * kFloat32Scale * context.volume_factor);
		*right_channel++ = static_cast<int32_t>(input_samples[i * 2 + 1] * kFloat32Scale * context.volume_factor);
	}
}

void DataConverter<PackedFormat::INTERLEAVED, PackedFormat::INTERLEAVED>::convert(int16_t* output, const float* input, const AudioConvertContext& context) {
	XAMP_ASSUME(input != nullptr);
	XAMP_ASSUME(output != nullptr);

	auto input_samples = std::span{ input, context.convert_size * AudioFormat::kMaxChannel };
	auto output_samples = std::span{ output, input_samples.size() };
	for (size_t i = 0; i < input_samples.size(); ++i) {
		output_samples[i] = static_cast<int16_t>(input_samples[i] * kFloat16Scale * context.volume_factor);
	}
}

void DataConverter<PackedFormat::INTERLEAVED, PackedFormat::INTERLEAVED>::convertToInt24(int24_t* output, const int32_t* input, const AudioConvertContext& context) {
	XAMP_ASSUME(input != nullptr);
	XAMP_ASSUME(output != nullptr);

	auto input_samples = std::span{ input, context.convert_size * AudioFormat::kMaxChannel };
	auto output_samples = std::span{ output, input_samples.size() };
	for (size_t i = 0; i < input_samples.size(); ++i) {
		output_samples[i] = input_samples[i] >> 8;
	}
}

void DataConverter<PackedFormat::INTERLEAVED, PackedFormat::INTERLEAVED>::convertToInt32(int32_t* output, const float* input, const AudioConvertContext& context) {
	XAMP_ASSUME(input != nullptr);
	XAMP_ASSUME(output != nullptr);

	auto input_samples = std::span{ input, context.convert_size * AudioFormat::kMaxChannel };
	auto output_samples = std::span{ output, input_samples.size() };
	for (size_t i = 0; i < input_samples.size(); ++i) {
		output_samples[i] = static_cast<int32_t>(input_samples[i] * kFloat32Scale * context.volume_factor);
	}
}

void DataConverter<PackedFormat::INTERLEAVED, PackedFormat::INTERLEAVED>::convertToInt2432(int32_t* output, const float* input, const AudioConvertContext& context) {
	XAMP_ASSUME(input != nullptr);
	XAMP_ASSUME(output != nullptr);

	auto input_samples = std::span{ input, context.convert_size * AudioFormat::kMaxChannel };
	auto output_samples = std::span{ output, input_samples.size() };
	for (size_t i = 0; i < input_samples.size(); ++i) {
		output_samples[i] = static_cast<int32_t>(input_samples[i] * kFloat24Scale * context.volume_factor) << 8;
	}
}

#endif


AudioConvertContext::AudioConvertContext() = default;

AudioConvertContext makeConvert(size_t convert_size) {
    AudioConvertContext context;
    context.convert_size = convert_size;
    return context;
}

AudioConverter::AudioConverter() = default;
void AudioConverter::convert(void* data, const void* buffer, const AudioConvertContext& context) {
	XAMP_ASSERT(impl_ != nullptr);
	std::invoke(impl_, data, buffer, context);
}

void AudioConverter::setFormat(uint32_t bit_per_sample, bool is_2432_format) {
	if (bit_per_sample != 16) {
		if (!is_2432_format) {
			switch (bit_per_sample) {
			case 24:
				// The input buffer is expected to be a 32-bit sample container for 24-bit output.
				impl_ = [](void* data, const void* buffer, const AudioConvertContext& context) {
					DataConverter<PackedFormat::INTERLEAVED, PackedFormat::INTERLEAVED>::convertToInt24(
						static_cast<int24_t*>(data),
						(int32_t*)buffer,
						context);
					};
				break;
			case 32:
				impl_ = [](void* data, const void* buffer, const AudioConvertContext& context) {
					DataConverter<PackedFormat::INTERLEAVED, PackedFormat::INTERLEAVED>::convertToInt32(
						static_cast<int32_t*>(data),
						(const float*)buffer,
						context);
					};
				break;
			}
		}
		else {
			impl_ = [](void* data, const void* buffer, const AudioConvertContext& context) {
				DataConverter<PackedFormat::INTERLEAVED, PackedFormat::INTERLEAVED>::convertToInt2432(
					static_cast<int32_t*>(data),
					(const float*)buffer,
					context);
				};
		}
	}
	else {
		impl_ = [](void* data, const void* buffer, const AudioConvertContext& context) {
			DataConverter<PackedFormat::INTERLEAVED, PackedFormat::INTERLEAVED>::convert(
				static_cast<int16_t*>(data),
				(const float*)buffer,
				context);
			};
	}
}

XAMP_BASE_NAMESPACE_END
