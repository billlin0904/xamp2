//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/idspmanager.h>

#include <base/memory.h>
#include <base/buffer.h>
#include <base/uuidof.h>
#include <base/stl.h>
#include <base/audiobuffer.h>

XAMP_BASE_NAMESPACE_BEGIN
class Logger;
XAMP_BASE_NAMESPACE_END

XAMP_STREAM_NAMESPACE_BEGIN

class DSPManager : public IDSPManager {
public:
    DSPManager();

    XAMP_DISABLE_COPY(DSPManager)

	void initialize(const Property& config) override;

    bool processDSP(const float* samples, uint32_t num_samples, AudioBuffer<std::byte>& fifo) override;

    void addPreDSP(ScopedPtr<IAudioProcessor> processor) override;

    void addPostDSP(ScopedPtr<IAudioProcessor> processor) override;

    IDSPManager& addParametricEq() override;

    IDSPManager& setParametricEq(bool enabled, const EqSettings& settings, const Property& config) override;

    IDSPManager& removeParametricEq() override;

    IDSPManager& removeSampleRateConverter() override;

    void setSampleWriter(ScopedPtr<ISampleWriter> writer = nullptr) override;

    [[nodiscard]] bool isEnableSampleRateConverter() const override;

    [[nodiscard]] bool canProcess() const override;

    [[nodiscard]] bool contains(const Uuid& type) const override;
private:
    void addOrReplace(ScopedPtr<IAudioProcessor> processor, std::vector<ScopedPtr<IAudioProcessor>>& dsp_chain);

    bool process(const float* samples, uint32_t num_samples, AudioBuffer<std::byte>& fifo);

    bool defaultProcess(const float* samples, uint32_t num_samples, AudioBuffer<std::byte>& fifo);

    using DspIterator = std::vector<ScopedPtr<IAudioProcessor>>::iterator;
    using ConstDspIterator = std::vector<ScopedPtr<IAudioProcessor>>::const_iterator;

    template <typename TDSP>
    DspIterator find(DspIterator begin,
        DspIterator end) {
        auto itr = std::find_if(begin, end, [](auto const& processor) {
            return processor->getTypeId() == XAMP_UUID_OF(TDSP);
            });
        return itr;
    }

    template <typename Func>
    [[nodiscard]] bool contains(Func &&func) const {
        if (findIf(pre_dsp_.begin(), pre_dsp_.end(), func) != pre_dsp_.end()) {
            return true;
        }
        return findIf(post_dsp_.begin(), post_dsp_.end(), func) != post_dsp_.end();
    }

    template <typename Func>
    [[nodiscard]] ConstDspIterator findIf(ConstDspIterator begin, ConstDspIterator end, Func &&func) const {
        auto itr = std::find_if(begin, end, [&](auto const& processor) {
            return func(processor->getTypeId());
            });
        return itr;
    }

    template <typename TDSP>
    std::optional<TDSP*> getDSP(
        DspIterator begin,
        DspIterator end
    ) {
        auto itr = find<TDSP>(begin, end);
        if (itr == end) {
            return std::nullopt;
        }
        return std::optional<TDSP*>{ dynamic_cast<TDSP*>((*itr).get()) };
    }

    template <typename TDSP>
    void removePreDSP() {
        auto itr = find<TDSP>(pre_dsp_.begin(), pre_dsp_.end());
        if (itr != pre_dsp_.end()) {
            pre_dsp_.erase(itr);
        }
    }

    template <typename TDSP>
    void removePostDSP() {
        auto itr = find<TDSP>(post_dsp_.begin(), post_dsp_.end());
        if (itr != post_dsp_.end()) {
            post_dsp_.erase(itr);
        }
    }

    template <typename TDSP>
    std::optional<TDSP*> getPreDSP() {
        return getDSP<TDSP>(pre_dsp_.begin(), pre_dsp_.end());
    }

    template <typename TDSP>
    std::optional<TDSP*> getPostDSP() {
        return getDSP<TDSP>(post_dsp_.begin(), post_dsp_.end());
    }

    std::vector<ScopedPtr<IAudioProcessor>> pre_dsp_;
    std::vector<ScopedPtr<IAudioProcessor>> post_dsp_;
    ScopedPtr<ISampleWriter> sample_writer_;
    Buffer<float> pre_dsp_buffer_;
    Buffer<float> post_dsp_buffer_;
    std::shared_ptr<Logger> logger_;
    Property config_;
    std::move_only_function<bool(float const*, uint32_t, AudioBuffer<std::byte>&)> dispatch_;
};

XAMP_STREAM_NAMESPACE_END
