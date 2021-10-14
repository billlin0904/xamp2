//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <base/uuid.h>
#include <base/buffer.h>
#include <stream/stream.h>

namespace xamp::stream {

using namespace xamp::base;

class XAMP_NO_VTABLE XAMP_STREAM_API IAudioProcessor {
public:
	XAMP_BASE_CLASS(IAudioProcessor)

	virtual void SetSampleRate(uint32_t sample_rate) = 0;

	virtual const Buffer<float>& Process(float const* samples, uint32_t num_samples) = 0;

	[[nodiscard]] virtual Uuid GetTypeId() const = 0;
	
protected:
	IAudioProcessor() = default;
};

}

