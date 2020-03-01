//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <vector>

#include <base/align_ptr.h>
#include <base/audiobuffer.h>
#include <base/audioformat.h>
#include <player/player.h>

namespace xamp::player {

using namespace xamp::base;

enum class SoxrQuality {
	LOW,
	MQ,
	HQ,	
	VHQ,
};

enum class SoxrPhase {
	LINEAR_PHASE = 0,
	INTERMEDIATE_PHASE,
	MINIMUM_PHASE,	
};

class XAMP_PALYER_API SoxrResampler {
public:
	SoxrResampler();

	~SoxrResampler();

	static void LoadSoxrLib();

	void SetSteepFilter(bool enable);

	void SetQuality(SoxrQuality quality);

	void SetPhase(SoxrPhase phase);

	void Start(int32_t input_samplerate, int32_t num_channels, int32_t output_samplerate);

	bool Process(const float* samples, int32_t num_sample, AudioBuffer<int8_t> &buffer);
private:
	class SoxrResamplerImpl;
	AlignPtr<SoxrResamplerImpl> impl_;
};

}

