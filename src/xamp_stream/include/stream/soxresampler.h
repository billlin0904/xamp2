//=====================================================================================================================
// Copyright (c) 2018-2021 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/iaudioprocessor.h>

#include <base/uuidof.h>
#include <base/enum.h>
#include <base/align_ptr.h>
#include <base/audiobuffer.h>
#include <base/pimplptr.h>

XAMP_STREAM_NAMESPACE_BEGIN

XAMP_MAKE_ENUM(SoxrQuality,
    SINC_LOW,
    SINC_MQ,
    SINC_HQ,
    SINC_VHQ,
    SINC_UHQ)

XAMP_MAKE_ENUM(SoxrRollOff,
    ROLLOFF_SMALL,
    ROLLOFF_MEDIUM,
    ROLLOFF_NONE)

class XAMP_STREAM_API SoxrSampleRateConverter final : public IAudioProcessor {
    XAMP_DECLARE_MAKE_CLASS_UUID(SoxrSampleRateConverter, "F986498A-9678-456F-96A7-2F6C2E5D13CB")

public:
    XAMP_PIMPL(SoxrSampleRateConverter)

    SoxrSampleRateConverter();    

    void SetQuality(SoxrQuality quality);

    void SetStopBand(double stop_band);

    void SetPassBand(double pass_band);

    void SetPhase(int32_t phase);

    void SetRollOff(SoxrRollOff level);

    void SetDither(bool enable);

    void Start(const AnyMap& config) override;

    void Init(const AnyMap& config) override;

    bool Process(float const* samples, uint32_t num_samples, BufferRef<float>& output) override;

    [[nodiscard]] Uuid GetTypeId() const override;

    [[nodiscard]] std::string_view GetDescription() const noexcept override;

    void Flush() override;

private:
    class SoxrSampleRateConverterImpl;
    AlignPtr<SoxrSampleRateConverterImpl> impl_;
};

XAMP_STREAM_NAMESPACE_END
