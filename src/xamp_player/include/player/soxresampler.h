//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/enum.h>
#include <base/align_ptr.h>
#include <base/audiobuffer.h>
#include <player/samplerateconverter.h>
#include <player/player.h>

namespace xamp::player {

using namespace xamp::base;

MAKE_ENUM(SoxrQuality,
          LOW,
          MQ,
          HQ,
          VHQ,
          UHQ)

MAKE_ENUM(SoxrPhaseResponse,
          LINEAR_PHASE,
          INTERMEDIATE_PHASE,
          MINIMUM_PHASE)

class XAMP_PLAYER_API SoxrSampleRateConverter final : public SampleRateConverter {
public:
    static const std::string_view VERSION;

    SoxrSampleRateConverter();

    ~SoxrSampleRateConverter() override;

    static void LoadSoxrLib();

    void SetSteepFilter(bool enable);

    void SetQuality(SoxrQuality quality);

    void SetPhase(SoxrPhaseResponse phase);

    void SetStopBand(double stopband);

    void SetPassBand(double passband);

    void SetDither(bool enable);

    std::string_view GetDescription() const noexcept override;

    void Start(uint32_t input_samplerate, uint32_t num_channels, uint32_t output_samplerate) override;

    bool Process(float const * samples, uint32_t num_sample, AudioBuffer<int8_t> &buffer) override;

    void Flush() override;

    AlignPtr<SampleRateConverter> Clone() override;

private:
    class SoxrResamplerImpl;
    AlignPtr<SoxrResamplerImpl> impl_;
};

}

