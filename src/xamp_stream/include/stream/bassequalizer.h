//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <array>
#include <base/audiobuffer.h>
#include <base/align_ptr.h>
#include <stream/stream.h>
#include <stream/equalizer.h>

namespace xamp::stream {

class XAMP_STREAM_API BassEqualizer final : public Equalizer {
public:
	BassEqualizer();

	XAMP_PIMPL(BassEqualizer)

	void Start(uint32_t num_channels, uint32_t input_samplerate) override;

	void SetEQ(uint32_t band, float gain) override;

	bool Process(float const* sample_buffer, uint32_t num_samples, AudioBuffer<int8_t>& buffer) override;
private:
	class BassEqualizerImpl;
	AlignPtr<BassEqualizerImpl> impl_;
};

}
