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

XAMP_BASE_NAMESPACE_BEGIN

struct XAMP_BASE_API AudioConvertContext {
    AudioConvertContext();
	float volume_factor{1.0};
    size_t cache_volume{0};
    size_t convert_size{0};
};

class XAMP_BASE_API AudioConverter {
public:
	AudioConverter();

	void setFormat(uint32_t bit_per_sample, bool is_2432_format);
	
	void convert(void* data, const void* buffer, const AudioConvertContext& context);
private:
	std::move_only_function<void(void*, const void*, const AudioConvertContext&)> impl_;
};

XAMP_BASE_API AudioConvertContext makeConvert(size_t convert_size);

XAMP_BASE_API void convertInt8ToInt8SSE(const int8_t* input, int8_t* left_ptr, int8_t* right_ptr, size_t frames);

XAMP_BASE_API void convertFloatToFloatSSE(const float* input, float* left_ptr, float* right_ptr, size_t frames);

XAMP_BASE_API void convertFloatToInt16SSE(const float* input, int16_t* left_ptr, int16_t* right_ptr, size_t frames);

XAMP_BASE_API void convertFloatToInt24(const float* input, int32_t* left_ptr, int32_t* right_ptr, size_t frames);

XAMP_BASE_API void convertFloatToInt16(const float* input, int16_t* left_ptr, int16_t* right_ptr, size_t frames);

XAMP_BASE_API void convertFloatToInt32SSE(const float* input, int32_t* left_ptr, int32_t* right_ptr, size_t frames, float volume = 1.0f);

XAMP_BASE_API void convertFloatToInt24SSE(const float* input, int32_t* left_ptr, int32_t* right_ptr, size_t frames);

template <PackedFormat InputFormat, PackedFormat OutputFormat>
struct DataConverter {};

template <>
struct DataConverter<PackedFormat::INTERLEAVED, PackedFormat::PLANAR> {
	static XAMP_BASE_API void convert(int8_t* output, const int8_t* input, const AudioConvertContext& context);

	static XAMP_BASE_API void convert(int32_t* output, const float* input, const AudioConvertContext& context);
};

template <>
struct DataConverter<PackedFormat::INTERLEAVED, PackedFormat::INTERLEAVED> {
	static XAMP_BASE_API void convert(int16_t* output, const float* input, const AudioConvertContext& context);

	static XAMP_BASE_API void convertToInt24(int24_t* output, const int32_t* input, const AudioConvertContext& context);

	static XAMP_BASE_API void convertToInt32(int32_t* output, const float* input, const AudioConvertContext& context);

	static XAMP_BASE_API void convertToInt2432(int32_t* output, const float* input, const AudioConvertContext& context);
};

XAMP_BASE_NAMESPACE_END
