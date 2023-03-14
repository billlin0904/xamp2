//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/stream.h>
#include <stream/iaudioprocessor.h>

#include <base/enum.h>
#include <base/uuidof.h>
#include <base/align_ptr.h>
#include <base/pimplptr.h>

namespace xamp::stream {

XAMP_MAKE_ENUM(FilterTypes,
    FT_LOW_PASS,
    FT_HIGH_PASS,
    FT_HIGH_BAND_PASS,
    FT_HIGH_BAND_PASS_Q,
    FT_NOTCH,
    FT_ALL_PASS,
    FT_ALL_PEAKING_EQ,
    FT_LOW_SHELF,
    FT_LOW_HIGH_SHELF
);

class BassParametricEq final : public IAudioProcessor {
    XAMP_DECLARE_MAKE_CLASS_UUID(BassParametricEq, "EBFA0111-594F-4F9D-9131-256451C3BF46")

public:
    BassParametricEq();

    XAMP_PIMPL(BassParametricEq)

    void Start(const AnyMap& config) override;

    void Init(const AnyMap& config) override;

    void SetBand(FilterTypes filter, uint32_t band, uint32_t center, uint32_t band_width, float gain, float Q, float S);

    bool Process(float const* samples, uint32_t num_samples, BufferRef<float>& out) override;

    uint32_t Process(float const* samples, float* out, uint32_t num_samples) override;

    Uuid GetTypeId() const override;

    [[nodiscard]] std::string_view GetDescription() const noexcept override;

    void Flush() override;
private:
    class BassParametricEqImpl;
    PimplPtr<BassParametricEqImpl> impl_;
};

}

