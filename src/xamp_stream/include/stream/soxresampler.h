//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/iaudioprocessor.h>

#include <base/enum.h>
#include <base/memory.h>
#include <base/audiobuffer.h>
#include <base/memory.h>

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

    void initialize(const Property& config) override;

    XAMP_PIMPL(SoxrSampleRateConverter)

    void setQuality(SoxrQuality quality);

    void setStopBand(double stop_band);

    void setPassBand(double pass_band);

    void setPhase(int32_t phase);

    void setRollOff(SoxrRollOff level);

    void setDither(bool enable);

    [[nodiscard]] bool process(float const* samples, size_t num_samples, BufferRef<float>& output) override;

private:
    class SoxrSampleRateConverterImpl;
    ScopedPtr<SoxrSampleRateConverterImpl> impl_;
};

XAMP_STREAM_NAMESPACE_END
