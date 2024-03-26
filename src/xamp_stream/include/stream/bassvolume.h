//=====================================================================================================================
// Copyright (c) 2018-2024 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/uuidof.h>
#include <base/memory.h>
#include <base/pimplptr.h>

#include <stream/stream.h>
#include <stream/iaudioprocessor.h>

XAMP_STREAM_NAMESPACE_BEGIN

class BassVolume final : public IAudioProcessor {
    XAMP_DECLARE_MAKE_CLASS_UUID(BassVolume, "83d25234-5484-45a3-bda8-baf35541f9d2")

public:
    constexpr static auto Description = std::string_view("BassVolume");

    BassVolume();

    XAMP_PIMPL(BassVolume)

    void Start(const AnyMap& config) override;

    void Init(const AnyMap& config) override;

    bool Process(float const * samples, size_t num_samples, BufferRef<float>& out) override;

    [[nodiscard]] Uuid GetTypeId() const override;

    [[nodiscard]] std::string_view GetDescription() const noexcept override;

private:
    class BassVolumeImpl;
    AlignPtr<BassVolumeImpl> impl_;
};

XAMP_STREAM_NAMESPACE_END