//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/stream.h>

#include <base/pimplptr.h>
#include <base/stl.h>

XAMP_STREAM_NAMESPACE_BEGIN

class XAMP_STREAM_API Ebur128Reader final {
public:
	Ebur128Reader();

	XAMP_PIMPL(Ebur128Reader)

	void SetSampleRate(uint32_t sample_rate);

	void Process(float const * samples, size_t num_sample) const;

	[[nodiscard]] double GetIntegratedLoudness() const;

	[[nodiscard]] double GetTruePeek() const;

    [[nodiscard]] double GetSamplePeak() const;

	[[nodiscard]] void* GetNativeHandle() const;

	static double GetEbur128Gain(double lufs, double targetdb);

    static double GetIntegratedMultipleLoudness(const Vector<Ebur128Reader>& scanners);

private:
	class Ebur128ReaderImpl;
	AlignPtr<Ebur128ReaderImpl> impl_;
};

XAMP_STREAM_NAMESPACE_END
