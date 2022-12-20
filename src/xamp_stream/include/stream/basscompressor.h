//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/iaudioprocessor.h>

#include <base/uuidof.h>
#include <base/align_ptr.h>

namespace xamp::stream {

class BassCompressor final : public IAudioProcessor {
    DECLARE_XAMP_MAKE_CLASS_UUID(BassCompressor, "263079D0-FDD4-46DF-9BB3-71821AF95EDB")

public:
    BassCompressor();

    XAMP_PIMPL(BassCompressor)

    void Start(const AnyMap& config) override;

    void Init(const AnyMap& config) override;

    bool Process(float const * samples, uint32_t num_samples, BufferRef<float>& out) override;

    uint32_t Process(float const* samples, float* out, uint32_t num_samples) override;

    [[nodiscard]] Uuid GetTypeId() const override;

    [[nodiscard]] std::string_view GetDescription() const noexcept override;

    void Flush() override;
private:
    class BassCompressorImpl;
    AlignPtr<BassCompressorImpl> impl_;
};

}