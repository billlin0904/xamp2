//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <base/base.h>
#include <stream/stream.h>

#include <base/audiobuffer.h>
#include <base/audioformat.h>
#include <base/dsdsampleformat.h>
#include <base/uuid.h>

namespace xamp::stream {

class XAMP_STREAM_API IDSPManager {
public:
    XAMP_BASE_CLASS(IDSPManager)

    virtual void Init(AudioFormat input_format, AudioFormat output_format, DsdModes dsd_mode, uint32_t sample_size) = 0;

    /*
     * return true (fetch more data).
     */
    virtual bool ProcessDSP(const float* samples, uint32_t num_samples, AudioBuffer<int8_t>& fifo) = 0;

    virtual void AddPreDSP(AlignPtr<IAudioProcessor> processor) = 0;

    virtual void AddPostDSP(AlignPtr<IAudioProcessor> processor) = 0;

    virtual void EnableDSP(bool enable = true) = 0;

    virtual void RemovePreDSP(Uuid const& id) = 0;

    virtual void RemovePostDSP(Uuid const& id) = 0;

    virtual void SetEq(uint32_t band, float gain, float Q) = 0;

    virtual void SetEq(EQSettings const& settings) = 0;

    virtual void SetPreamp(float preamp) = 0;

    virtual void SetReplayGain(double volume) = 0;

    virtual bool IsEnableDSP() const noexcept = 0;

    virtual bool IsEnableSampleRateConverter() const = 0;

    virtual bool CanProcessFile() const noexcept = 0;

    virtual void Flush() = 0;
protected:
    IDSPManager() = default;
};

}

