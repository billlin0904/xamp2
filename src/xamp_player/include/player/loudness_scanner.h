//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/align_ptr.h>
#include <player/player.h>

namespace xamp::player {

class XAMP_PLAYER_API LoudnessScanner final {
public:
	explicit LoudnessScanner(uint32_t sample_rate);

	XAMP_PIMPL(LoudnessScanner)

	void Process(float const * samples, size_t num_sample);

	[[nodiscard]] double GetLoudness() const;

	[[nodiscard]] double GetTruePeek() const;

    [[nodiscard]] double GetSamplePeak() const;

    void* GetNativeHandle() const;

	static double GetEbur128Gain(double loudness, double targetdb);

    static double GetMultipleEbur128Gain(std::vector<AlignPtr<LoudnessScanner>> &scanners);
private:
	class LoudnessScannerImpl;
	AlignPtr<LoudnessScannerImpl> impl_;
};

}
