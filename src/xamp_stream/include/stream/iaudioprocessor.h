//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/uuid.h>
#include <base/buffer.h>
#include <stream/stream.h>

namespace xamp::stream {

class XAMP_NO_VTABLE XAMP_STREAM_API IAudioProcessor {
public:
	XAMP_BASE_CLASS(IAudioProcessor)

    virtual void Start(uint32_t samplerate) = 0;

    virtual void Process(float const* samples, uint32_t num_samples, Buffer<float>& out) = 0;

    [[nodiscard]] virtual Uuid GetTypeId() const = 0;

protected:
	IAudioProcessor() = default;
};

}

