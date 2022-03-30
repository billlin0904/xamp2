//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/enum.h>
#include <base/align_ptr.h>
#include <base/audiobuffer.h>
#include <stream/iaudioprocessor.h>

namespace xamp::stream {

MAKE_XAMP_ENUM(SoxrQuality,
          LOW,
          MQ,
          HQ,
          VHQ,
          UHQ)

MAKE_XAMP_ENUM(SoxrRollOff,
          ROLLOFF_SMALL,
          ROLLOFF_MEDIUM,
          ROLLOFF_NONE)

class XAMP_STREAM_API SoxrSampleRateConverter final : public IAudioProcessor {
public:
    constexpr static auto Id = std::string_view("F986498A-9678-456F-96A7-2F6C2E5D13CB");
    static const std::string_view VERSION;

    XAMP_PIMPL(SoxrSampleRateConverter)

    SoxrSampleRateConverter();    

    void SetSteepFilter(bool enable);

    void SetQuality(SoxrQuality quality);

    void SetStopBand(double stop_band);

    void SetPassBand(double pass_band);

    void SetPhase(int32_t phase);

    void SetRollOff(SoxrRollOff level);

    void SetDither(bool enable);

    void Init(uint32_t input_sample_rate);

    void Start(uint32_t output_sample_rate) override;

    bool Process(float const* samples, uint32_t num_samples, BufferRef<float>& output) override;

    [[nodiscard]] Uuid GetTypeId() const override;

    [[nodiscard]] std::string_view GetDescription() const noexcept override;

    void Flush() override;

private:
    class SoxrSampleRateConverterImpl;
    AlignPtr<SoxrSampleRateConverterImpl> impl_;
};

XAMP_STREAM_API void LoadSoxrLib();

}

