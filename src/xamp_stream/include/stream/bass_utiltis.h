//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <functional>
#include <base/buffer.h>
#include <stream/basslib.h>

namespace xamp {
namespace stream {
	class BassFileStream;
}
}

namespace xamp::stream::BassUtiltis {

uint32_t Process(BassStreamHandle& stream, float const* samples, float * out, uint32_t num_samples);
bool Process(BassStreamHandle& stream, float const* samples, uint32_t num_samples, BufferRef<float>& out);
void Encode(BassFileStream &stream, std::function<bool(uint32_t) > const& progress);

}

