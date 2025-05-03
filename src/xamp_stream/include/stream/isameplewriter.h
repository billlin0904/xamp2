//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/audiobuffer.h>
#include <stream/stream.h>
#include <base/uuidof.h>

XAMP_STREAM_NAMESPACE_BEGIN

class XAMP_STREAM_API XAMP_NO_VTABLE ISampleWriter {
public:
    XAMP_BASE_CLASS(ISampleWriter)

    XAMP_NO_DISCARD virtual bool Process(BufferRef<float> const &input, AudioBuffer<std::byte>& buffer) = 0;
	
    XAMP_NO_DISCARD virtual bool Process(float const * samples, size_t num_sample, AudioBuffer<std::byte>& buffer) = 0;

    XAMP_NO_DISCARD virtual Uuid GetTypeId() const = 0;
protected:
    ISampleWriter() = default;
};

XAMP_STREAM_NAMESPACE_END
