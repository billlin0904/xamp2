//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/buffer.h>
#include <base/memory.h>
#include <functional>

#include <stream/stream.h>

XAMP_STREAM_NAMESPACE_BEGIN

class XAMP_STREAM_API Ebur128Scanner final {
public:
	explicit Ebur128Scanner(uint32_t sample_rate);

	XAMP_PIMPL(Ebur128Scanner)

	void Process(float const* samples, size_t num_sample);

	[[nodiscard]] double GetLoudness() const;

	[[nodiscard]] double GetTruePeek() const;

	[[nodiscard]] double GetSamplePeak() const;

	[[nodiscard]] void* GetNativeHandle() const;

	static double GetMultipleLoudness(std::vector<Ebur128Scanner>& scanners);

	static void LoadEbur128Lib();
private:
	class Ebur128ScannerImpl;
	ScopedPtr<Ebur128ScannerImpl> impl_;
};

XAMP_STREAM_NAMESPACE_END