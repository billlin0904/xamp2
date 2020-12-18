//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <array>
#include <base/base.h>
#include <base/stl.h>
#include <base/audioformat.h>

namespace xamp::player {

using namespace xamp::base;

class DitheringEffect {
public:
	DitheringEffect() {
		Reset();
	}

	void Reset() {
        phase_ = 0;
        buffer_.fill(0);
	}

    void Process(float * samples, uint32_t num_sample) {
        Reset();

#define DITHER_NOISE (rand() / (float)RAND_MAX - 0.5f)		
		for (size_t i = 0; i < num_sample; ++i) {
            float r = DITHER_NOISE + DITHER_NOISE;

			auto sample = samples[i];
			if (sample != sample) {
				sample = 0;
			}
			
            float xe = sample + buffer_[phase_] * kShapedBs[0]
                + buffer_[(phase_ - 1) & kBufMask] * kShapedBs[1]
                + buffer_[(phase_ - 2) & kBufMask] * kShapedBs[2]
                + buffer_[(phase_ - 3) & kBufMask] * kShapedBs[3]
                + buffer_[(phase_ - 4) & kBufMask] * kShapedBs[4];

			float result = xe + r;

            phase_ = (phase_ + 1) & kBufMask;
            buffer_[phase_] = xe - lrintf(result);

			samples[i] = result;
		}
	}
	
private:
    static constexpr int kBufferSize = 8;
    static constexpr int kBufMask = 7;
    static constexpr float kShapedBs[] = { 2.033f, -2.165f, 1.959f, -1.590f, 0.6149f };
	
    int32_t phase_;
    std::array<float, kBufferSize> buffer_;
};

}

