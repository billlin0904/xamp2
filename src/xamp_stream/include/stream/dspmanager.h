//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/idspmanager.h>

#include <base/align_ptr.h>
#include <base/buffer.h>
#include <base/uuidof.h>
#include <base/stl.h>
#include <base/audiobuffer.h>

XAMP_STREAM_NAMESPACE_BEGIN

class DSPManager : public IDSPManager {
public:
    DSPManager();

    XAMP_DISABLE_COPY(DSPManager)

	void Init(const AnyMap& config) override;

    bool ProcessDSP(const float* samples, uint32_t num_samples, AudioBuffer<int8_t>& fifo) override;

    bool ProcessDSP(const float* samples, uint32_t num_samples, float* out) override;

    void AddPreDSP(AlignPtr<IAudioProcessor> processor) override;

    void AddPostDSP(AlignPtr<IAudioProcessor> processor) override;

    IDSPManager& AddEqualizer() override;

    IDSPManager& AddCompressor() override;

    IDSPManager& AddVolumeControl() override;

    IDSPManager& RemoveEqualizer() override;

    IDSPManager& RemoveVolumeControl() override;

    IDSPManager& RemoveSampleRateConverter() override;

    IDSPManager& RemoveCompressor() override;

    void SetSampleWriter(AlignPtr<ISampleWriter> writer = nullptr) override;

    bool IsEnableSampleRateConverter() const override;

    bool IsEnablePcm2DsdConverter() const override;

    bool CanProcess() const noexcept override;

    void Flush() override;

private:
    void AddOrReplace(AlignPtr<IAudioProcessor> processor, Vector<AlignPtr<IAudioProcessor>>& dsp_chain);

    bool Process(const float* samples, uint32_t num_samples, AudioBuffer<int8_t>& fifo);

    bool DefaultProcess(const float* samples, uint32_t num_samples, AudioBuffer<int8_t>& fifo);

    using DspIterator = Vector<AlignPtr<IAudioProcessor>>::iterator;
    using ConstDspIterator = Vector<AlignPtr<IAudioProcessor>>::const_iterator;

    template <typename TDSP>
    DspIterator Find(DspIterator begin,
        DspIterator end) {
        auto itr = std::find_if(begin, end, [](auto const& processor) {
            return processor->GetTypeId() == XAMP_UUID_OF(TDSP);
            });
        return itr;
    }

    template <typename Func>
    bool Contains(Func func) const {
        if (FindIf(pre_dsp_.begin(), pre_dsp_.end(), func) == pre_dsp_.end()) {
            return FindIf(post_dsp_.begin(), post_dsp_.end(), func) != post_dsp_.end();
        }
        return false;
    }

    template <typename Func>
    ConstDspIterator FindIf(ConstDspIterator begin, ConstDspIterator end, Func func) const {
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
        return dynamic_cast<TDSP*>((*itr).get());
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

    Vector<AlignPtr<IAudioProcessor>> pre_dsp_;
    Vector<AlignPtr<IAudioProcessor>> post_dsp_;
    AlignPtr<ISampleWriter> sample_writer_;
    Buffer<float> pre_dsp_buffer_;
    Buffer<float> post_dsp_buffer_;
    LoggerPtr logger_;
    AnyMap config_;
    std::function<bool(float const*, size_t, AudioBuffer<int8_t>&)> dispatch_;
};

XAMP_STREAM_NAMESPACE_END