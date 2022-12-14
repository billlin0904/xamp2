//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once


#include <stream/stream.h>
#include <stream/iaudioprocessor.h>

#include <base/base.h>
#include <base/audiobuffer.h>

namespace xamp::stream {

class XAMP_STREAM_API XAMP_NO_VTABLE IDSPManager {
public:
    XAMP_BASE_CLASS(IDSPManager)

	virtual void Init(const DspConfig& config) = 0;

    // note: return true (fetch more data).
    [[nodiscard]] virtual bool ProcessDSP(const float* samples, uint32_t num_samples, AudioBuffer<int8_t>& fifo) = 0;

    [[nodiscard]] virtual bool ProcessDSP(const float* samples, uint32_t num_samples, float* out) = 0;

    virtual void AddPreDSP(AlignPtr<IAudioProcessor> processor) = 0;

    virtual void AddPostDSP(AlignPtr<IAudioProcessor> processor) = 0;

    virtual IDSPManager& AddEqualizer() = 0;

    virtual IDSPManager& AddCompressor() = 0;

    virtual IDSPManager& AddVolume() = 0;

    virtual IDSPManager& RemoveEqualizer() = 0;

    virtual IDSPManager& RemoveVolume() = 0;

    virtual IDSPManager& RemoveSampleRateConverter() = 0;

    virtual void SetSampleWriter(AlignPtr<ISampleWriter> writer = nullptr) = 0;

    [[nodiscard]] virtual bool IsEnableSampleRateConverter() const = 0;

    [[nodiscard]] virtual bool IsEnablePcm2DsdConverter() const = 0;

    [[nodiscard]] virtual bool CanProcess() const noexcept = 0;

    virtual void Flush() = 0;
protected:
    IDSPManager() = default;
};

}

