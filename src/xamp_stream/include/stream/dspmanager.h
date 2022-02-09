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
#include <stream/eqsettings.h>

namespace xamp::stream {

class XAMP_STREAM_API DSPManager {
public:
    DSPManager();

    XAMP_DISABLE_COPY(DSPManager)

    void Init(AudioFormat input_format, AudioFormat output_format, DsdModes dsd_mode, uint32_t sample_size);

    /*
     * return true (fetch more data).
     */
    bool ProcessDSP(const float* samples, uint32_t num_samples, AudioBuffer<int8_t>& fifo);

    void AddPreDSP(AlignPtr<IAudioProcessor> processor);

    void AddPostDSP(AlignPtr<IAudioProcessor> processor);

    void EnableDSP(bool enable = true);

    void RemovePreDSP(Uuid const& id);

    void RemovePostDSP(Uuid const& id);

    void SetEq(uint32_t band, float gain, float Q);

    void SetEq(EQSettings const& settings);

    void SetPreamp(float preamp);

    void SetReplayGain(double volume);

    bool IsEnableDSP() const noexcept;

    bool IsEnableSampleRateConverter() const;

    bool CanProcessFile() const noexcept;

    void Flush();

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
    AlignPtr<ISampleRateConverter> converter_;
    Buffer<float> temp_;
    Buffer<float> dsp_buffer_;
    std::shared_ptr<spdlog::logger> logger_;
};

}
