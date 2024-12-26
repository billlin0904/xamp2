//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/basslib.h>

#include <base/buffer.h>
#include <functional>

XAMP_STREAM_NAMESPACE_BEGIN

class BassFileStream;

XAMP_STREAM_NAMESPACE_END

XAMP_STREAM_UTIL_NAMESPACE_BEGIN 

uint32_t ReadStream(const BassStreamHandle& stream, float const* samples, float* out, size_t num_samples);

bool ReadStream(const BassStreamHandle& stream, float const* samples, size_t num_samples, BufferRef<float>& out);

void Encode(FileStream& stream, std::function<bool(uint32_t) > const& progress);

XAMP_STREAM_UTIL_NAMESPACE_END

