//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/stream.h>
#include <stream/iaudioprocessor.h>

#include <base/uuidof.h>
#include <base/align_ptr.h>
#include <base/pimplptr.h>

XAMP_STREAM_NAMESPACE_BEGIN

inline constexpr auto kMaxSuperEqBand = 18;

class SuperEqEqualizer final : public IAudioProcessor {
    XAMP_DECLARE_MAKE_CLASS_UUID(SuperEqEqualizer, "3A8DE658-B3B6-0D06-13AA-AEB71B3C314E")

public:
    SuperEqEqualizer();

    XAMP_PIMPL(SuperEqEqualizer)

	void Start(const AnyMap& config) override;

    void Init(const AnyMap& config) override;

    bool Process(float const* samples, uint32_t num_samples, BufferRef<float>& out) override;

    [[nodiscard]] Uuid GetTypeId() const override;

    [[nodiscard]] std::string_view GetDescription() const noexcept override;

    void Flush() override;

private:
    class SuperEqEqualizerImpl;
    AlignPtr<SuperEqEqualizerImpl> impl_;
};

XAMP_STREAM_NAMESPACE_END
