//=====================================================================================================================
// Copyright (c) 2018-2021 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/iaudioprocessor.h>

#include <base/uuidof.h>
#include <base/align_ptr.h>
#include <base/audiobuffer.h>
#include <base/pimplptr.h>

XAMP_STREAM_NAMESPACE_BEGIN

XAMP_MAKE_ENUM(SrcQuality,
    SINC_LQ,
    SINC_MQ,
    SINC_HQ)

class XAMP_STREAM_API SrcSampleRateConverter final : public IAudioProcessor {
    XAMP_DECLARE_MAKE_CLASS_UUID(SrcSampleRateConverter, "00DA7FC2-3033-461A-B9AE-41A1AA80747D")

public:
    XAMP_PIMPL(SrcSampleRateConverter)

	SrcSampleRateConverter();

    void SetQuality(SrcQuality quality);

    void Start(const AnyMap& config) override;

    void Init(const AnyMap& config) override;

    bool Process(float const* samples, uint32_t num_samples, BufferRef<float>& output) override;

    [[nodiscard]] Uuid GetTypeId() const override;

    [[nodiscard]] std::string_view GetDescription() const noexcept override;

    void Flush() override;

private:
    class SrcSampleRateConverterImpl;
    AlignPtr<SrcSampleRateConverterImpl> impl_;
};

XAMP_STREAM_NAMESPACE_END
