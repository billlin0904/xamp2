//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <vector>
#include <base/align_ptr.h>
#include <stream/stream.h>
#include <stream/audioprocessor.h>

namespace xamp::stream {

inline constexpr double kMaxTruePeak = -1.0;

using namespace xamp::base;

class XAMP_STREAM_API Compressor : public AudioProcessor {
public:
    constexpr static auto Id = std::string_view("263079D0-FDD4-46DF-9BB3-71821AF95EDB");
	
    struct Parameters {
        float gain{ 0 };
        float threshold{ 0 };
        float ratio{ 0 };
        float attack{ 0 };
        float release{ 0 };
    };
	
    Compressor();

    XAMP_PIMPL(Compressor)

    void SetSampleRate(uint32_t sample_rate) override;

    void Prepare(Parameters const &parameters);

    const std::vector<float>& Process(float const * samples, uint32_t num_samples) override;

    Uuid GetTypeId() const override;

private:
    class CompressorImpl;
    AlignPtr<CompressorImpl> impl_;
};

}

