//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <array>

#include <base/base.h>
#include <base/audioformat.h>
#include <base/int24.h>
#include <base/memory.h>

#ifdef XAMP_OS_WIN
#define XAMP_VECTOR_CALL __vectorcall
#else
#define XAMP_VECTOR_CALL __attribute__((vectorcall))
#endif

// XAMP_IS_LITTLE_ENDIAN
// XAMP_IS_BIG_ENDIAN
#define XAMP_IS_LITTLE_ENDIAN 1
//#define XAMP_IS_BIG_ENDIAN 1

XAMP_BASE_NAMESPACE_BEGIN

inline constexpr float kFloat16Scale { 32767.f };
inline constexpr float kFloat24Scale { 8388607.f };
// note: 必須要加上.f否則是round to 2147483648.
inline constexpr float kFloat32Scale { 2147483647.f };
inline constexpr float kMaxFloatSample { 1.0F };
inline constexpr float kMinFloatSample { -1.0F };

struct XAMP_BASE_API AudioConvertContext {
    AudioConvertContext();
	float volume_factor{1.0};
    size_t cache_volume{0};
    size_t convert_size{0};
};

class XAMP_BASE_API AudioConverter {
public:
	AudioConverter();

	void SetFormat(uint32_t bit_per_sample, bool is_2432_format);
	
	void convert(void* data, const void* buffer, const AudioConvertContext& context);
private:
	std::move_only_function<void(void*, const void*, const AudioConvertContext&)> impl_;
};

XAMP_BASE_API AudioConvertContext MakeConvert(size_t convert_size);

#ifdef XAMP_OS_WIN

XAMP_BASE_API void ConvertInt8ToInt8SSE(const int8_t* input, int8_t* left_ptr, int8_t* right_ptr, size_t frames);

XAMP_BASE_API void ConvertFloatToFloatSSE(const float* input, float* left_ptr, float* right_ptr, size_t frames);

XAMP_BASE_API void ConvertFloatToInt16SSE(const float* input, int16_t* left_ptr, int16_t* right_ptr, size_t frames);

XAMP_BASE_API void ConvertFloatToInt24(const float* input, int32_t* left_ptr, int32_t* right_ptr, size_t frames);

XAMP_BASE_API void ConvertFloatToInt16(const float* input, int16_t* left_ptr, int16_t* right_ptr, size_t frames);

XAMP_BASE_API void ConvertFloatToInt32SSE(const float* input, int32_t* left_ptr, int32_t* right_ptr, size_t frames, float volume = 1.0f);

XAMP_BASE_API void ConvertFloatToInt24SSE(const float* input, int32_t* left_ptr, int32_t* right_ptr, size_t frames);

template <PackedFormat InputFormat, PackedFormat OutputFormat>
struct DataConverter {};

template <>
struct DataConverter<PackedFormat::INTERLEAVED, PackedFormat::PLANAR> {
	static XAMP_BASE_API void Convert(int8_t* output, const int8_t* input, const AudioConvertContext& context);

	static XAMP_BASE_API void Convert(int32_t* output, const float* input, const AudioConvertContext& context);
};

template <>
struct DataConverter<PackedFormat::INTERLEAVED, PackedFormat::INTERLEAVED> {
	static XAMP_BASE_API void Convert(int16_t* output, const float* input, const AudioConvertContext& context);

	static XAMP_BASE_API void ConvertToInt24(int24_t* output, const int32_t* input, const AudioConvertContext& context);

	static XAMP_BASE_API void ConvertToInt32(int32_t* output, const float* input, const AudioConvertContext& context);

	static XAMP_BASE_API void ConvertToInt2432(int32_t* output, const float* input, const AudioConvertContext& context);
};

#endif


XAMP_BASE_NAMESPACE_END
