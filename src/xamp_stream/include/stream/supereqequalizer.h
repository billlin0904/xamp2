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

#ifdef XAMP_OS_WIN

XAMP_STREAM_NAMESPACE_BEGIN

inline constexpr auto kMaxSuperEqBand = 18;

class SuperEqEqualizer final : public IAudioProcessor {
    XAMP_DECLARE_MAKE_CLASS_UUID(SuperEqEqualizer, "3A8DE658-B3B6-0D06-13AA-AEB71B3C314E")

public:
    SuperEqEqualizer();

    XAMP_PIMPL(SuperEqEqualizer)

	void Start(const AnyMap& config) override;

    void Initialize(const AnyMap& config) override;

    bool Process(float const* samples, size_t num_samples, BufferRef<float>& out) override;

    [[nodiscard]] Uuid GetTypeId() const override;

    [[nodiscard]] std::string_view GetDescription() const noexcept override;

private:
    class SuperEqEqualizerImpl;
    AlignPtr<SuperEqEqualizerImpl> impl_;
};

XAMP_STREAM_NAMESPACE_END

#endif
