//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/isameplewriter.h>

#include <base/dsdsampleformat.h>
#include <base/uuidof.h>

#include <functional>

XAMP_STREAM_NAMESPACE_BEGIN

class XAMP_STREAM_API DsdModeSampleWriter final : public ISampleWriter {
    XAMP_DECLARE_MAKE_CLASS_UUID(DsdModeSampleWriter, "490A3F39-6829-49B7-822C-DD7D72D7E57F")

public:
    explicit DsdModeSampleWriter(DsdModes dsd_mode, uint8_t sample_size);

    [[nodiscard]] bool Process(float const * sample_buffer, size_t num_samples, AudioBuffer<int8_t>& buffer) override;

    [[nodiscard]] bool Process(BufferRef<float> const& input, AudioBuffer<int8_t>& buffer) override;

private:
    bool ProcessNativeDsd(int8_t const * sample_buffer, size_t num_samples, AudioBuffer<int8_t>& buffer);

    bool ProcessPcm(int8_t const * sample_buffer, size_t num_samples, AudioBuffer<int8_t>& buffer);

	DsdModes dsd_mode_;
    uint8_t sample_size_;
    std::function<bool(int8_t const* , size_t , AudioBuffer<int8_t>& )> dispatch_;
};

XAMP_STREAM_NAMESPACE_END

