//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <vector>
#include <base/align_ptr.h>
#include <player/player.h>

namespace xamp::player {

class XAMP_PLAYER_API Ebur128ReplayGainScanner final {
public:
	explicit Ebur128ReplayGainScanner(uint32_t sample_rate);

	XAMP_PIMPL(Ebur128ReplayGainScanner)

	void Process(float const * samples, size_t num_sample);

	[[nodiscard]] double GetLoudness() const;

	[[nodiscard]] double GetTruePeek() const;

    [[nodiscard]] double GetSamplePeak() const;

    void* GetNativeHandle() const;

	static double GetEbur128Gain(double loudness, double targetdb);

    static double GetMultipleLoudness(std::vector<Ebur128ReplayGainScanner>& scanners);

	static void LoadEbur128Lib();
private:
	class Ebur128ReplayGainScannerImpl;
	AlignPtr<Ebur128ReplayGainScannerImpl> impl_;
};

}
