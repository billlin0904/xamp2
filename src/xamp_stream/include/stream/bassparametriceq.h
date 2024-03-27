//=====================================================================================================================
// Copyright (c) 2018-2024 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/enum.h>
#include <base/uuidof.h>
#include <base/memory.h>
#include <base/pimplptr.h>

#include <stream/stream.h>
#include <stream/eqsettings.h>
#include <stream/iaudioprocessor.h>

XAMP_STREAM_NAMESPACE_BEGIN

class BassParametricEq final : public IAudioProcessor {
    XAMP_DECLARE_MAKE_CLASS_UUID(BassParametricEq, "EBFA0111-594F-4F9D-9131-256451C3BF46")

public:
    BassParametricEq();

    XAMP_PIMPL(BassParametricEq)

    void Start(const AnyMap& config) override;

    void Initialize(const AnyMap& config) override;

    void SetBand(EQFilterTypes filter, uint32_t band, uint32_t center, uint32_t band_width, float gain, float Q, float S);

    bool Process(float const* samples, size_t num_samples, BufferRef<float>& out) override;

    Uuid GetTypeId() const override;

    [[nodiscard]] std::string_view GetDescription() const noexcept override;

private:
    void SetEq(EqSettings const& settings);

    class BassParametricEqImpl;
    AlignPtr<BassParametricEqImpl> impl_;
};

XAMP_STREAM_NAMESPACE_END

