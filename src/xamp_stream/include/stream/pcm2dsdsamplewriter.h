//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/stream.h>
#include <stream/isameplewriter.h>

namespace xamp::stream {

MAKE_XAMP_ENUM(DsdTimes,
	DSD_TIME_4X = 4, // DSD16
	DSD_TIME_5X,     // DSD32
	DSD_TIME_6X,	 // DSD64
	DSD_TIME_7X,	 // DSD128
	DSD_TIME_8X,
	DSD_TIME_9X,
	DSD_TIME_10X,
	DSD_TIME_11X,
	)

class XAMP_STREAM_API Pcm2DsdSampleWriter final : public ISampleWriter {
public:
	explicit Pcm2DsdSampleWriter(DsdTimes dsd_times);

	void Init(uint32_t input_sample_rate);

	uint32_t GetDsdSampleRate() const;

	uint32_t GetDsdSpeed() const;

	XAMP_PIMPL(Pcm2DsdSampleWriter)

	[[nodiscard]] std::string_view GetDescription() const noexcept override;

	[[nodiscard]] bool Process(BufferRef<float> const& input, AudioBuffer<int8_t>& buffer) override;

	bool Process(float const* samples, size_t num_samples, AudioBuffer<int8_t>& buffer) override;

private:
	class Pcm2DsdSampleWriterImpl;
	AlignPtr<Pcm2DsdSampleWriterImpl> impl_;
};
	
}

