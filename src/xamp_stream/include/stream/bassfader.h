//=====================================================================================================================
// Copyright (c) 2018-2024 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/stream.h>
#include <stream/iaudioprocessor.h>

#include <base/uuidof.h>
#include <base/memory.h>
#include <base/pimplptr.h>

XAMP_STREAM_NAMESPACE_BEGIN

class XAMP_STREAM_API BassFader final : public IAudioProcessor {
    XAMP_DECLARE_MAKE_CLASS_UUID(BassFader, "6981FB2E-E133-4B33-9378-F23628EEE4FE")

public:
    constexpr static auto Description = std::string_view("BassFader");

    BassFader();

    XAMP_PIMPL(BassFader)

    void Start(const AnyMap& config) override;

    void Init(const AnyMap& config) override;

    void SetTime(float current, float target, float fdade_time);

    bool Process(float const * samples, uint32_t num_samples, BufferRef<float>& out) override;

    [[nodiscard]] Uuid GetTypeId() const override;

    [[nodiscard]] std::string_view GetDescription() const noexcept override;

private:
    class BassFaderImpl;
    AlignPtr<BassFaderImpl> impl_;
};

XAMP_STREAM_NAMESPACE_END
