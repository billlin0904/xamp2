//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once


#include <stream/stream.h>
#include <stream/iaudioprocessor.h>
#include <stream/isameplewriter.h>
#include <stream/eqsettings.h>

#include <base/base.h>
#include <base/pcm.h>
#include <base/audiobuffer.h>

XAMP_STREAM_NAMESPACE_BEGIN

class XAMP_STREAM_API XAMP_NO_VTABLE IDSPManager {
public:
    XAMP_BASE_CLASS(IDSPManager)

	// Change only while playback is stopped. The strict path bypasses every DSP and writer.
    virtual void setBitPerfect(bool enabled) = 0;
    [[nodiscard]] virtual bool isBitPerfect() const = 0;

    virtual void processPcm(const xamp::pcm::Block& block, AudioBuffer<std::byte>& fifo) = 0;

	virtual void initialize(const Property& config) = 0;

    // note: return true (fetch more data).
    [[nodiscard]] virtual bool processDSP(const float* samples, uint32_t num_samples, AudioBuffer<std::byte>& fifo) = 0;

    virtual void addPreDSP(ScopedPtr<IAudioProcessor> processor) = 0;

    virtual void addPostDSP(ScopedPtr<IAudioProcessor> processor) = 0;

    virtual IDSPManager& addParametricEq() = 0;

    virtual IDSPManager& setParametricEq(bool enabled, const EqSettings& settings, const Property& config) = 0;

    virtual IDSPManager& removeParametricEq() = 0;

    virtual IDSPManager& removeSampleRateConverter() = 0;

    virtual void setSampleWriter(ScopedPtr<ISampleWriter> writer = nullptr) = 0;

    [[nodiscard]] virtual bool isEnableSampleRateConverter() const = 0;

    [[nodiscard]] virtual bool canProcess() const = 0;

    [[nodiscard]] virtual bool contains(const Uuid &type) const = 0;

protected:
    IDSPManager() = default;
};

XAMP_STREAM_NAMESPACE_END
