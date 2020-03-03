//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/align_ptr.h>
#include <base/audiobuffer.h>
#include <base/audioformat.h>
#include <player/player.h>
#include <player/resampler.h>

namespace xamp::player {

class XAMP_PLAYER_API CdspResampler : public Resampler {
public:
	CdspResampler();

	XAMP_PIMPL(CdspResampler)

	static void LoadCdspLib();

	void Start(int32_t input_samplerate, int32_t num_channels, int32_t output_samplerate, int32_t max_sample) override;

	bool Process(const float* samples, int32_t num_sample, AudioBuffer<int8_t>& buffer) override;
	
private:
	class CdspResamplerImpl;
	AlignPtr<CdspResamplerImpl> impl_;
};

}
