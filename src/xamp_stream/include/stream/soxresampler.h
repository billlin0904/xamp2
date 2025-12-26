//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/iaudioprocessor.h>

#include <base/enum.h>
#include <base/memory.h>
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
    SoxrSampleRateConverter();

	XAMP_DECLARE_UUID_CLASS(SoxrSampleRateConverter)

    void Initialize(const AnyMap& config) override;

    XAMP_PIMPL(SoxrSampleRateConverter)

    void SetQuality(SoxrQuality quality);

    void SetStopBand(double stop_band);

    void SetPassBand(double pass_band);

    void SetPhase(int32_t phase);

    void SetRollOff(SoxrRollOff level);

    void SetDither(bool enable);

    [[nodiscard]] bool Process(float const* samples, size_t num_samples, BufferRef<float>& output) override;

private:
    class SoxrSampleRateConverterImpl;
    ScopedPtr<SoxrSampleRateConverterImpl> impl_;
};

XAMP_STREAM_NAMESPACE_END
