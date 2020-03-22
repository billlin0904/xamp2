//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <player/resampler.h>
#include <base/dsdsampleformat.h>

namespace xamp::player {

class NullResampler : public Resampler {
public:
	explicit NullResampler(DsdModes dsd_mode, int32_t sample_size)
		: dsd_mode_(dsd_mode)
		, sample_size_(sample_size) {
	}

	void Start(int32_t input_samplerate, int32_t num_channels, int32_t output_samplerate, int32_t max_sample) override {
	}

	bool Process(const float* samples, int32_t num_samples, AudioBuffer<int8_t>& buffer) {
		return true;
	}

	std::string_view GetDescription() const noexcept override {
		return "None";
	}
private:
	DsdModes dsd_mode_;
	int32_t sample_size_;
};

}

