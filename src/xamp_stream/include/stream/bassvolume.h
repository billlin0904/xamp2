//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/stream.h>
#include <stream/iaudioprocessor.h>

#include <base/uuidof.h>
#include <base/align_ptr.h>

namespace xamp::stream {

class BassVolume final : public IAudioProcessor {
    XAMP_DECLARE_MAKE_CLASS_UUID(BassVolume, "83d25234-5484-45a3-bda8-baf35541f9d2")

public:
    BassVolume();

    XAMP_PIMPL(BassVolume)

    void Start(const AnyMap& config) override;

    void Init(const AnyMap& config) override;

    bool Process(float const * samples, uint32_t num_samples, BufferRef<float>& out) override;

    uint32_t Process(float const* samples, float* out, uint32_t num_samples) override;

    [[nodiscard]] Uuid GetTypeId() const override;

    [[nodiscard]] std::string_view GetDescription() const noexcept override;

    void Flush() override;

private:
    class BassVolumeImpl;
    AlignPtr<BassVolumeImpl> impl_;
};

}