//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/basslib.h>

#include <base/buffer.h>
#include <functional>

namespace xamp {
namespace stream {
	class BassFileStream;
}
}

namespace xamp::stream::BassUtiltis {

uint32_t Process(BassStreamHandle& stream, float const* samples, float * out, uint32_t num_samples);
bool Process(BassStreamHandle& stream, float const* samples, uint32_t num_samples, BufferRef<float>& out);
void Encode(FileStream& stream, std::function<bool(uint32_t) > const& progress);

}

