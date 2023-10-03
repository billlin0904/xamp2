//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once


#include <stream/stream.h>
#include <stream/iaudioprocessor.h>

#include <base/base.h>
#include <base/audiobuffer.h>

XAMP_STREAM_NAMESPACE_BEGIN

class XAMP_STREAM_API XAMP_NO_VTABLE IDSPManager {
public:
    XAMP_BASE_CLASS(IDSPManager)

	virtual void Init(const AnyMap& config) = 0;

    // note: return true (fetch more data).
    [[nodiscard]] virtual bool ProcessDSP(const float* samples, uint32_t num_samples, AudioBuffer<int8_t>& fifo) = 0;

    virtual void AddPreDSP(AlignPtr<IAudioProcessor> processor) = 0;

    virtual void AddPostDSP(AlignPtr<IAudioProcessor> processor) = 0;

    virtual IDSPManager& AddEqualizer() = 0;

    virtual IDSPManager& AddParametricEq() = 0;

    virtual IDSPManager& AddCompressor() = 0;

    virtual IDSPManager& AddVolumeControl() = 0;

    virtual IDSPManager& RemoveEqualizer() = 0;

    virtual IDSPManager& RemoveParametricEq() = 0;

    virtual IDSPManager& RemoveVolumeControl() = 0;

    virtual IDSPManager& RemoveCompressor() = 0;

    virtual IDSPManager& RemoveSampleRateConverter() = 0;

    virtual void SetSampleWriter(AlignPtr<ISampleWriter> writer = nullptr) = 0;

    [[nodiscard]] virtual bool IsEnableSampleRateConverter() const = 0;

    [[nodiscard]] virtual bool CanProcess() const noexcept = 0;

    virtual void Flush() = 0;
protected:
    IDSPManager() = default;
};

XAMP_STREAM_NAMESPACE_END

