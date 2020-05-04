//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <player/resampler.h>
#include <base/dsdsampleformat.h>

namespace xamp::player {

class XAMP_PLAYER_API NullResampler final : public Resampler {
public:
    explicit NullResampler(DsdModes dsd_mode, uint32_t sample_size);

    void Start(uint32_t, uint32_t, uint32_t, uint32_t) override;

    bool Process(const float* sample_buffer, uint32_t num_samples, AudioBuffer<int8_t>& buffer) override;

    [[nodiscard]] std::string_view GetDescription() const noexcept override;

    void Flush() override;
private:
    bool ProcessNativeDsd(const int8_t* sample_buffer, uint32_t num_samples, AudioBuffer<int8_t>& buffer);

    bool ProcessPcm(const int8_t* sample_buffer, uint32_t num_samples, AudioBuffer<int8_t>& buffer);

    typedef bool (NullResampler::*ProcessDispatch)(const int8_t*, uint32_t, AudioBuffer<int8_t>&);
	DsdModes dsd_mode_;
    uint32_t sample_size_;
	ProcessDispatch process_;
};

}

