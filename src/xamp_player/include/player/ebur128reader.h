//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <player/player.h>

#include <base/pimplptr.h>
#include <base/stl.h>

namespace xamp::player {

class XAMP_PLAYER_API Ebur128Reader final {
public:
	Ebur128Reader();

	XAMP_PIMPL(Ebur128Reader)

	void SetSampleRate(uint32_t sample_rate);

	void Process(float const * samples, size_t num_sample);

	[[nodiscard]] double GetLoudness() const;

	[[nodiscard]] double GetTruePeek() const;

    [[nodiscard]] double GetSamplePeak() const;

    void* GetNativeHandle() const;

	static double GetEbur128Gain(double loudness, double targetdb);

    static double GetMultipleLoudness(const Vector<Ebur128Reader>& scanners);

	static void LoadEbur128Lib();
private:
	class Ebur128ReaderImpl;
	PimplPtr<Ebur128ReaderImpl> impl_;
};

}
