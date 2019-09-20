//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/audioformat.h>

namespace xamp::base {

struct ConvertContext {
};

inline ConvertContext MakeConvert(const AudioFormat &source_format, const AudioFormat& dest_format, uint32_t frame_size) {
	return ConvertContext();
}

template <InterleavedFormat SrcFormat, InterleavedFormat DestFormat>
struct DataConverter {
	static void Convert2432Bit(int32_t* dest, const float* src, ConvertContext context) {
	}
};

template <>
struct DataConverter<InterleavedFormat::INTERLEAVED, InterleavedFormat::INTERLEAVED> {
	static void Convert2432Bit(int32_t* dest, const float* src, ConvertContext context) {
	}
};

}
