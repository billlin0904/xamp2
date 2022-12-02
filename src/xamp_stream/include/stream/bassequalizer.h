//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <array>
#include <base/audiobuffer.h>
#include <base/align_ptr.h>
#include <stream/stream.h>
#include <stream/iaudioprocessor.h>

namespace xamp::stream {

class XAMP_STREAM_API BassEqualizer final : public IAudioProcessor {
public:
    BassEqualizer();

    XAMP_PIMPL(BassEqualizer)

    void Start(const DspConfig& config) override;

    void Init(const DspConfig& config) override;

    void SetEQ(uint32_t band, float gain, float Q);

    void SetEQ(EQSettings const &settingss);

    bool Process(float const* samples, uint32_t num_samples, BufferRef<float>& out) override;

    Uuid GetTypeId() const override;

    void SetPreamp(float preamp);

    [[nodiscard]] std::string_view GetDescription() const noexcept override;

    void Flush() override;
private:
    class BassEqualizerImpl;
    AlignPtr<BassEqualizerImpl> impl_;
};
XAMP_MAKE_CLASS_UUID(BassEqualizer, "FCC73B23-6806-44CD-882D-EA21A3482F51")

}
