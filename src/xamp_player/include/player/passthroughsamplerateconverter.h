//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <player/samplerateconverter.h>
#include <base/dsdsampleformat.h>

namespace xamp::player {

class XAMP_PLAYER_API PassThroughSampleRateConverter final : public SampleRateConverter {
public:
    explicit PassThroughSampleRateConverter(DsdModes dsd_mode, uint8_t sample_size);

    void Start(uint32_t, uint32_t num_channels, uint32_t output_sample_rate) override;

    bool Process(float const * sample_buffer, size_t num_samples, AudioBuffer<int8_t>& buffer) override;

    bool Process(float const* samples, size_t num_sample, SampleWriter& writer) override;

    bool Process(Buffer<float> const& input, AudioBuffer<int8_t>& buffer) override;

    [[nodiscard]] std::string_view GetDescription() const noexcept override;

    void Flush() override;

    AlignPtr<SampleRateConverter> Clone() override;
private:
    bool ProcessNativeDsd(int8_t const * sample_buffer, size_t num_samples, AudioBuffer<int8_t>& buffer);

    bool ProcessPcm(int8_t const * sample_buffer, size_t num_samples, AudioBuffer<int8_t>& buffer);

    typedef bool (PassThroughSampleRateConverter::*ProcessDispatch)(int8_t const *, size_t, AudioBuffer<int8_t>&);
	DsdModes dsd_mode_;
    uint8_t sample_size_;
    uint32_t output_sample_rate_;
	ProcessDispatch process_;
};

}

