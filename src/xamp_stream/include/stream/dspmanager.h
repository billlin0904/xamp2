//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/idspmanager.h>

#include <base/memory.h>
#include <base/buffer.h>
#include <base/uuidof.h>
#include <base/stl.h>
#include <base/audiobuffer.h>

XAMP_STREAM_NAMESPACE_BEGIN

class DSPManager : public IDSPManager {
public:
    DSPManager();

    XAMP_DISABLE_COPY(DSPManager)

	void Initialize(const AnyMap& config) override;

    bool ProcessDSP(const float* samples, uint32_t num_samples, AudioBuffer<std::byte>& fifo) override;

    void AddPreDSP(ScopedPtr<IAudioProcessor> processor) override;

    void AddPostDSP(ScopedPtr<IAudioProcessor> processor) override;

    IDSPManager& AddEqualizer() override;

    IDSPManager& AddParametricEq() override;

    IDSPManager& AddCompressor() override;

    IDSPManager& RemoveEqualizer() override;

    IDSPManager& RemoveParametricEq() override;

    IDSPManager& RemoveSampleRateConverter() override;

    IDSPManager& RemoveCompressor() override;

    void SetSampleWriter(ScopedPtr<ISampleWriter> writer = nullptr) override;

    [[nodiscard]] bool IsEnableSampleRateConverter() const override;

    [[nodiscard]] bool CanProcess() const noexcept override;

    [[nodiscard]] bool Contains(const Uuid& type) const noexcept override;
private:
    void AddOrReplace(ScopedPtr<IAudioProcessor> processor, std::vector<ScopedPtr<IAudioProcessor>>& dsp_chain);

    bool Process(const float* samples, uint32_t num_samples, AudioBuffer<std::byte>& fifo);

    bool DefaultProcess(const float* samples, uint32_t num_samples, AudioBuffer<std::byte>& fifo);

    using DspIterator = std::vector<ScopedPtr<IAudioProcessor>>::iterator;
    using ConstDspIterator = std::vector<ScopedPtr<IAudioProcessor>>::const_iterator;

    template <typename TDSP>
    DspIterator Find(DspIterator begin,
        DspIterator end) {
        auto itr = std::find_if(begin, end, [](auto const& processor) {
            return processor->GetTypeId() == XAMP_UUID_OF(TDSP);
            });
        return itr;
    }

    template <typename Func>
    [[nodiscard]] bool Contains(Func &&func) const {
        if (FindIf(pre_dsp_.begin(), pre_dsp_.end(), func) == pre_dsp_.end()) {
            return FindIf(post_dsp_.begin(), post_dsp_.end(), func) != post_dsp_.end();
        }
        return false;
    }

    template <typename Func>
    [[nodiscard]] ConstDspIterator FindIf(ConstDspIterator begin, ConstDspIterator end, Func &&func) const {
        auto itr = std::find_if(begin, end, [&](auto const& processor) {
            return func(processor->GetTypeId());
            });
        return itr;
    }

    template <typename TDSP>
    std::optional<TDSP*> GetDSP(
        DspIterator begin,
        DspIterator end
    ) const {
        auto itr = Find<TDSP>(begin, end);
        if (itr == end) {
            return std::nullopt;
        }
        return CreateOptional<TDSP*>(dynamic_cast<TDSP*>((*itr).get()));
    }

    template <typename TDSP>
    void RemovePreDSP() {
        auto itr = Find<TDSP>(pre_dsp_.begin(), pre_dsp_.end());
        if (itr != pre_dsp_.end()) {
            pre_dsp_.erase(itr);
        }
    }

    template <typename TDSP>
    void RemovePostDSP() {
        auto itr = Find<TDSP>(post_dsp_.begin(), post_dsp_.end());
        if (itr != post_dsp_.end()) {
            post_dsp_.erase(itr);
        }
    }

    template <typename TDSP>
    std::optional<TDSP*> GetPreDSP() const {
        return GetDSP<TDSP>(pre_dsp_.begin(), pre_dsp_.end());
    }

    template <typename TDSP>
    std::optional<TDSP*> GetPostDSP() const {
        return GetDSP<TDSP>(post_dsp_.begin(), post_dsp_.end());
    }

    std::vector<ScopedPtr<IAudioProcessor>> pre_dsp_;
    std::vector<ScopedPtr<IAudioProcessor>> post_dsp_;
    ScopedPtr<ISampleWriter> sample_writer_;
    Buffer<float> pre_dsp_buffer_;
    Buffer<float> post_dsp_buffer_;
    LoggerPtr logger_;
    AnyMap config_;
    std::function<bool(float const*, uint32_t, AudioBuffer<std::byte>&)> dispatch_;
};

XAMP_STREAM_NAMESPACE_END
