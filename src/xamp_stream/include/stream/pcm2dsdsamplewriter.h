//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/stream.h>
#include <stream/dsd_times.h>
#include <stream/isameplewriter.h>

#include <base/platform.h>
#include <base/uuidof.h>
#include <base/dsdsampleformat.h>
#include <base/pimplptr.h>

namespace xamp::stream {

XAMP_MAKE_ENUM(Pcm2DsdConvertModes, 
	PCM2DSD_DSD_DOP,
	PCM2DSD_DSD_NATIVE)

class XAMP_STREAM_API Pcm2DsdSampleWriter final : public ISampleWriter {
	XAMP_DECLARE_MAKE_CLASS_UUID(Pcm2DsdSampleWriter, "1CDC7F5F-BF93-4CA3-ACAC-3FC2B04157D5")

public:
	explicit Pcm2DsdSampleWriter(DsdTimes dsd_times);

	void Init(uint32_t input_sample_rate, CpuAffinity affinity, Pcm2DsdConvertModes convert_mode);

	uint32_t GetDsdSampleRate() const;

	uint32_t GetDsdSpeed() const;

	uint32_t GetDataSize() const;

	XAMP_PIMPL(Pcm2DsdSampleWriter)

	[[nodiscard]] bool Process(BufferRef<float> const& input, AudioBuffer<int8_t>& buffer) override;

	bool Process(float const* samples, size_t num_samples, AudioBuffer<int8_t>& buffer) override;

private:
	class Pcm2DsdSampleWriterImpl;
	AlignPtr<Pcm2DsdSampleWriterImpl> impl_;
};
	
}

