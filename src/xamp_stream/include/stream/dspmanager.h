//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <vector>
#include <base/align_ptr.h>
#include <base/uuid.h>
#include <base/buffer.h>
#include <base/audiobuffer.h>
#include <base/logger.h>
#include <base/audioformat.h>
#include <base/dsdsampleformat.h>

#include <stream/stream.h>
#include <stream/idspmanager.h>
#include <stream/eqsettings.h>

namespace xamp::stream {

class DSPManager : public IDSPManager {
public:
    DSPManager();

    XAMP_DISABLE_COPY(DSPManager)

    void Init(AudioFormat input_format, AudioFormat output_format, DsdModes dsd_mode, uint32_t sample_size) override;

    bool ProcessDSP(const float* samples, uint32_t num_samples, AudioBuffer<int8_t>& fifo) override;

    void AddPreDSP(AlignPtr<IAudioProcessor> processor) override;

    void AddPostDSP(AlignPtr<IAudioProcessor> processor) override;

    void RemovePreDSP(Uuid const& id) override;

    void RemovePostDSP(Uuid const& id) override;

    void SetEq(EQSettings const& settings) override;

    void EnableVolumeLimiter(bool enable) override;

    void RemoveEq() override;

    void SetPreamp(float preamp) override;

    void SetReplayGain(double volume) override;

    void EnablePcm2DsdConvert(uint32_t dsd_times) override;

    void RemoveReplayGain() override;

    bool IsEnableSampleRateConverter() const override;

    bool CanProcessFile() const noexcept override;

    void Flush() override;

private:
    void AddOrReplace(AlignPtr<IAudioProcessor> processor, Vector<AlignPtr<IAudioProcessor>>& dsp_chain);

    void Remove(Uuid const& id, Vector<AlignPtr<IAudioProcessor>>& dsp_chain);

    bool ApplyDSP(const float* samples, uint32_t num_samples, AudioBuffer<int8_t>& fifo);

    using DspIterator = Vector<AlignPtr<IAudioProcessor>>::const_iterator;

    template <typename TDSP>
    std::optional<TDSP*> GetDSP(
        DspIterator begin,
        DspIterator end
    ) const {
        auto itr = std::find_if(begin, end, [](auto const& processor) {
            return processor->GetTypeId() == TDSP::Id;
            });
        if (itr == end) {
            return std::nullopt;
        }
        return dynamic_cast<TDSP*>((*itr).get());
    }

    template <typename TDSP>
    std::optional<TDSP*> GetPreDSP() const {
        return GetDSP<TDSP>(pre_dsp_.begin(), pre_dsp_.end());
    }

    template <typename TDSP>
    std::optional<TDSP*> GetPostDSP() const {
        return GetDSP<TDSP>(post_dsp_.begin(), post_dsp_.end());
    }

    uint32_t dsd_times_;
    double replay_gain_;
    DsdModes dsd_modes_;
    EQSettings eq_settings_;
    Vector<AlignPtr<IAudioProcessor>> pre_dsp_;
    Vector<AlignPtr<IAudioProcessor>> post_dsp_;
    AlignPtr<ISampleRateConverter> pcm2dsd_;
    AlignPtr<ISampleRateConverter> fifo_writer_;
    Buffer<float> pre_dsp_buffer_;
    Buffer<float> post_dsp_buffer_;
    std::shared_ptr<Logger> logger_;
};

}
