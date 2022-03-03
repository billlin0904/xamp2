//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
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

    /*
     * return true (fetch more data).
     */
    bool ProcessDSP(const float* samples, uint32_t num_samples, AudioBuffer<int8_t>& fifo) override;

    void AddPreDSP(AlignPtr<IAudioProcessor> processor) override;

    void AddPostDSP(AlignPtr<IAudioProcessor> processor) override;

    void EnableDSP(bool enable = true) override;

    void RemovePreDSP(Uuid const& id) override;

    void RemovePostDSP(Uuid const& id) override;

    void SetEq(uint32_t band, float gain, float Q) override;

    void SetEq(EQSettings const& settings) override;

    void SetPreamp(float preamp) override;

    void SetReplayGain(double volume) override;

    bool IsEnableDSP() const noexcept override;

    bool IsEnableSampleRateConverter() const override;

    bool CanProcessFile() const noexcept override;

    void Flush() override;

private:
    bool ApplyDSP(const float* samples, uint32_t num_samples, AudioBuffer<int8_t>& fifo);

    using DspIterator = std::vector<AlignPtr<IAudioProcessor>>::const_iterator;

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

    bool enable_processor_;
    double replay_gain_;
    DsdModes dsd_modes_;
    EQSettings eq_settings_;
    std::vector<AlignPtr<IAudioProcessor>> pre_dsp_;
    std::vector<AlignPtr<IAudioProcessor>> post_dsp_;
    AlignPtr<ISampleRateConverter> fifo_writer_;
    Buffer<float> pre_dsp_buffer_;
    Buffer<float> dsp_buffer_;
    std::shared_ptr<spdlog::logger> logger_;
};

}
