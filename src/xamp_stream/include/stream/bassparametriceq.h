//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/stream.h>
#include <stream/eqsettings.h>
#include <stream/iaudioprocessor.h>

#include <base/enum.h>
#include <base/uuidof.h>
#include <base/align_ptr.h>
#include <base/pimplptr.h>

namespace xamp::stream {

class BassParametricEq final : public IAudioProcessor {
    XAMP_DECLARE_MAKE_CLASS_UUID(BassParametricEq, "EBFA0111-594F-4F9D-9131-256451C3BF46")

public:
    BassParametricEq();

    XAMP_PIMPL(BassParametricEq)

    void Start(const AnyMap& config) override;

    void Init(const AnyMap& config) override;

    /*
     * @band_width: Bandwidth in octaves (0.1...4...n), Q is not in use (fBandwidth has priority over fQ). Default = 1 (0=not in use).
     *              The bandwidth in octaves (between -3 dB frequencies for for BANDPASS and NOTCH or between midpoint (dBgain/2) gain frequencies for PEAKINGEQ).
     * @center: Cut-off frequency (Center in PEAKINGEQ and Shelving filters) in Hz (1...info.freq/2). Default = 200Hz.
     * @gain: Gain in dB (-15...0...+15). Default 0dB (used only for PEAKINGEQ and Shelving filters).
     * @Q: The EE kinda definition (linear), if fBandwidth is not in use (0.1...1). Default = 0.0 (0=not in use).
     * @S: A shelf slope parameter (linear, used only with Shelving filters) (0.1...1). Default = 0.0.
     *     When fS=1, the shelf slope is as steep as you can get it and remain monotonically increasing or decreasing gain with frequency.
     */
    void SetBand(EQFilterTypes filter, uint32_t band, uint32_t center, uint32_t band_width, float gain, float Q, float S);

    bool Process(float const* samples, uint32_t num_samples, BufferRef<float>& out) override;

    uint32_t Process(float const* samples, float* out, uint32_t num_samples) override;

    Uuid GetTypeId() const override;

    [[nodiscard]] std::string_view GetDescription() const noexcept override;

    void Flush() override;
private:
    class BassParametricEqImpl;
    PimplPtr<BassParametricEqImpl> impl_;
};

}

