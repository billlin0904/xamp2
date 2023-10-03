//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/stream.h>
#include <stream/eqsettings.h>
#include <stream/iaudioprocessor.h>

#include <base/enum.h>
#include <base/uuidof.h>
#include <base/align_ptr.h>
#include <base/pimplptr.h>

XAMP_STREAM_NAMESPACE_BEGIN

class BassParametricEq final : public IAudioProcessor {
    XAMP_DECLARE_MAKE_CLASS_UUID(BassParametricEq, "EBFA0111-594F-4F9D-9131-256451C3BF46")

public:
    BassParametricEq();

    XAMP_PIMPL(BassParametricEq)

    void Start(const AnyMap& config) override;

    void Init(const AnyMap& config) override;

    void SetBand(EQFilterTypes filter, uint32_t band, uint32_t center, uint32_t band_width, float gain, float Q, float S);

    bool Process(float const* samples, uint32_t num_samples, BufferRef<float>& out) override;

    uint32_t Process(float const* samples, float* out, uint32_t num_samples) override;

    Uuid GetTypeId() const override;

    [[nodiscard]] std::string_view GetDescription() const noexcept override;

    void Flush() override;
private:
    void SetEq(EqSettings const& settings);

    class BassParametricEqImpl;
    PimplPtr<BassParametricEqImpl> impl_;
};

XAMP_STREAM_NAMESPACE_END

