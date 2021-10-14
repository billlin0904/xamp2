//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <vector>
#include <base/align_ptr.h>
#include <stream/stream.h>
#include <stream/iaudioprocessor.h>

namespace xamp::stream {

inline constexpr double kMaxTruePeak = -1.0;

using namespace xamp::base;

struct XAMP_STREAM_API CompressorParameters final {
    CompressorParameters() noexcept
        : gain(-1)
        , threshold(-1)
        , ratio(100)
        , attack(0.1)
        , release(50) {
    }

    // �A�Q�Q���Y�᪺������y�W�j�h�֭��q
    // Output gain in dB of signal after compression.
    float gain;
    // Point in dB at which compression begins, in decibels, in the range from -60 to 0.
    // ���q���e�A�i�`���Y����ɶ}�����Y�{��
    float threshold;
    // Compression ratio, in the range from 1 to 100.
    // ���Y�ɪ��j�z
    float ratio;
    // Time in ms before compression reaches its full value, in the range from 0.01 to 500.
    // ���q�W�LThreshold��A�A�Q���Y������ɭԶ}�l���Y�H
    float attack;
    // Time (speed) in ms at which compression is stopped after input drops below fThreshold, in the range from 50 to 3000.
    // ���n���Q���Y��ӭ��q�S�^���C��Threshold�A�A�Q���Y������ɭ԰������Y�H
    float release;
};

class XAMP_STREAM_API Compressor : public IAudioProcessor {
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

