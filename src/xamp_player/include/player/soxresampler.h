//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <vector>

#include <base/enum.h>
#include <base/align_ptr.h>
#include <base/audiobuffer.h>
#include <base/audioformat.h>
#include <player/resampler.h>
#include <player/player.h>

namespace xamp::player {

MAKE_ENUM(SoxrQuality,
	LOW,
	MQ,
	HQ,	
    VHQ)

MAKE_ENUM(SoxrPhaseResponse,
    LINEAR_PHASE,
    INTERMEDIATE_PHASE,
    MINIMUM_PHASE)

class XAMP_PLAYER_API SoxrResampler : public Resampler {
public:
	SoxrResampler();

	XAMP_PIMPL(SoxrResampler)

	static void LoadSoxrLib();

	void SetSteepFilter(bool enable);

	void SetQuality(SoxrQuality quality);

	void SetPhase(SoxrPhaseResponse phase);

	void SetStopBand(double stopband);

	void SetPassBand(double passband);

	std::string_view GetDescription() const noexcept override;

	void Start(int32_t input_samplerate, int32_t num_channels, int32_t output_samplerate, int32_t max_sample) override;

	bool Process(const float* samples, int32_t num_sample, AudioBuffer<int8_t> &buffer) override;
private:
	class SoxrResamplerImpl;
	AlignPtr<SoxrResamplerImpl> impl_;
};

}

