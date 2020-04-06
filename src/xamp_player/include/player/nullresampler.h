//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <player/resampler.h>
#include <base/dsdsampleformat.h>

namespace xamp::player {

class XAMP_PLAYER_API NullResampler final : public Resampler {
public:
    explicit NullResampler(DsdModes dsd_mode, uint32_t sample_size)
		: dsd_mode_(dsd_mode)
		, sample_size_(sample_size) {
	}

    void Start(uint32_t, uint32_t, uint32_t, uint32_t) override {
	}

    bool Process(const float*, uint32_t, AudioBuffer<int8_t>&) override {
		return true;
	}

	std::string_view GetDescription() const noexcept override {
		return "None";
	}
private:
	DsdModes dsd_mode_;
    uint32_t sample_size_;
};

}

