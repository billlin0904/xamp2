//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/stream.h>
#include <stream/isamplerateconverter.h>

namespace xamp::stream {

class Pcm2DsdConverter final : public ISampleRateConverter {
public:
	Pcm2DsdConverter(uint32_t output_sample_rate, uint32_t dsd_times);

	[[nodiscard]] std::string_view GetDescription() const noexcept override;

	[[nodiscard]] bool Process(BufferRef<float> const& input, AudioBuffer<int8_t>& buffer) override;

	bool Process(float const* samples, size_t num_sample, AudioBuffer<int8_t>& buffer) override;

private:
	std::vector<uint8_t> ProcessChannel(std::vector<double>& buffer, uint32_t sample_size, uint32_t sample_rate);

};
	
}

