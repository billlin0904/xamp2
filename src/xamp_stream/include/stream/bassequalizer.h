//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <array>
#include <base/audiobuffer.h>
#include <base/align_ptr.h>
#include <stream/stream.h>
#include <stream/iequalizer.h>

namespace xamp::stream {

class XAMP_STREAM_API BassEqualizer final : public IEqualizer {
public:
    BassEqualizer();

    XAMP_PIMPL(BassEqualizer)

    void Start(uint32_t samplerate) override;

    void SetEQ(uint32_t band, float gain, float Q) override;

    void SetEQ(EQSettings const &settingss) override;

    void Process(float const* samples, uint32_t num_samples, Buffer<float>& out) override;

    Uuid GetTypeId() const override;

    void SetPreamp(float preamp) override;
private:
    class BassEqualizerImpl;
    AlignPtr<BassEqualizerImpl> impl_;
};

}
