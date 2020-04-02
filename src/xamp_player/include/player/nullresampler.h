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
    explicit NullResampler(DsdModes dsd_mode, uint32_t sample_size)
		: dsd_mode_(dsd_mode)
		, sample_size_(sample_size) {
	}

    void Start(uint32_t input_samplerate, uint32_t num_channels, uint32_t output_samplerate, uint32_t max_sample) override {
	}

    bool Process(const float* samples, uint32_t num_samples, AudioBuffer<int8_t>& buffer) override {
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

