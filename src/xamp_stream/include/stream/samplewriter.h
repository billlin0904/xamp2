//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <functional>
#include <base/dsdsampleformat.h>
#include <stream/isameplewriter.h>

namespace xamp::stream {

class XAMP_STREAM_API SampleWriter final : public ISampleWriter {
public:
    explicit SampleWriter(DsdModes dsd_mode, uint8_t sample_size);

    bool Process(float const * sample_buffer, size_t num_samples, AudioBuffer<int8_t>& buffer) override;

    bool Process(BufferRef<float> const& input, AudioBuffer<int8_t>& buffer) override;

    [[nodiscard]] std::string_view GetDescription() const noexcept override;

private:
    bool ProcessNativeDsd(int8_t const * sample_buffer, size_t num_samples, AudioBuffer<int8_t>& buffer);

    bool ProcessPcm(int8_t const * sample_buffer, size_t num_samples, AudioBuffer<int8_t>& buffer);

	DsdModes dsd_mode_;
    uint8_t sample_size_;
    uint32_t output_sample_rate_;
    std::function<bool(int8_t const* , size_t , AudioBuffer<int8_t>& )> dispatch_;
};

}

