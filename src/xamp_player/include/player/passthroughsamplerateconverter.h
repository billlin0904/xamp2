//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <player/samplerateconverter.h>
#include <base/dsdsampleformat.h>

namespace xamp::player {

class XAMP_PLAYER_API PassThroughSampleRateConverter final : public SampleRateConverter {
public:
    explicit PassThroughSampleRateConverter(DsdModes dsd_mode, uint8_t sample_size);

    void Start(uint32_t, uint32_t, uint32_t) override;

    bool Process(float const * sample_buffer, uint32_t num_samples, AudioBuffer<int8_t>& buffer) override;

    [[nodiscard]] std::string_view GetDescription() const noexcept override;

    void Flush() override;

    AlignPtr<SampleRateConverter> Clone() override;
private:
    bool ProcessNativeDsd(int8_t const * sample_buffer, uint32_t num_samples, AudioBuffer<int8_t>& buffer);

    bool ProcessPcm(int8_t const * sample_buffer, uint32_t num_samples, AudioBuffer<int8_t>& buffer);

    typedef bool (PassThroughSampleRateConverter::*ProcessDispatch)(int8_t const *, uint32_t, AudioBuffer<int8_t>&);
	DsdModes dsd_mode_;
    uint8_t sample_size_;
	ProcessDispatch process_;
};

}

