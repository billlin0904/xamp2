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
		mTriangleState = 0;
		mPhase = 0;
		mBuffer.fill(0);
	}

	void Process(float * samples, uint32_t num_sample) {
#define DITHER_NOISE (rand() / (float)RAND_MAX - 0.5f)

		float r = DITHER_NOISE + DITHER_NOISE;
		
		for (size_t i = 0; i < num_sample; ++i) {
			auto sample = samples[i];
			if (sample != sample) {
				sample = 0;
			}
			
			float xe = sample + mBuffer[mPhase] * SHAPED_BS[0]
				+ mBuffer[(mPhase - 1) & BUF_MASK] * SHAPED_BS[1]
				+ mBuffer[(mPhase - 2) & BUF_MASK] * SHAPED_BS[2]
				+ mBuffer[(mPhase - 3) & BUF_MASK] * SHAPED_BS[3]
				+ mBuffer[(mPhase - 4) & BUF_MASK] * SHAPED_BS[4];

			float result = xe + r;

			mPhase = (mPhase + 1) & BUF_MASK;
			mBuffer[mPhase] = xe - lrintf(result);

			samples[i] = result;
		}
	}
	
private:
	static constexpr int BUF_SIZE = 8;
	static constexpr int BUF_MASK = 7;
	static constexpr float SHAPED_BS[] = { 2.033f, -2.165f, 1.959f, -1.590f, 0.6149f };
	
	int mPhase;
	float mTriangleState;
	std::array<float, BUF_SIZE> mBuffer;
};

}

