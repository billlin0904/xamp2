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

struct XAMP_STREAM_API CompressorParameters final {
    CompressorParameters() noexcept
        : gain(-1)
        , threshold(0)
        , ratio(100)
        , attack(0.01f)
        , release(200) {
    }

    // Output gain in dB of signal after compression.
    float gain;
    // Point in dB at which compression begins, in decibels, in the range from -60 to 0.
    float threshold;
    // Compression ratio, in the range from 1 to 100.
    float ratio;
    // Time in ms before compression reaches its full value, in the range from 0.01 to 500.
    float attack;
    // Time (speed) in ms at which compression is stopped after input drops below fThreshold, in the range from 50 to 3000. 
    float release;
};

class XAMP_STREAM_API Compressor : public AudioProcessor {
public:
    constexpr static auto Id = std::string_view("263079D0-FDD4-46DF-9BB3-71821AF95EDB");    
	
    Compressor();

    XAMP_PIMPL(Compressor)

    void SetSampleRate(uint32_t sample_rate) override;

    void Init(CompressorParameters const &parameters = CompressorParameters());

    const Buffer<float>& Process(float const * samples, uint32_t num_samples) override;

    [[nodiscard]] Uuid GetTypeId() const override;

private:
    class CompressorImpl;
    AlignPtr<CompressorImpl> impl_;
};

}
