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
#include <spdlog/logger.h>
#include <stream/stream.h>

#include "eqsettings.h"
#include "base/audioformat.h"
#include "base/dsdsampleformat.h"

namespace xamp::stream {

class XAMP_STREAM_API DSPManager {
public:
    DSPManager();

    void InitDsp(AudioFormat input_format, AudioFormat output_format, DsdModes dsd_mode, uint32_t sample_size);

    bool ProcessDSP(const float* samples, uint32_t num_samples, AudioBuffer<int8_t>& fifo);

    void AddPreDSP(AlignPtr<IAudioProcessor> processor);

    void AddPostDSP(AlignPtr<IAudioProcessor> processor);

    void EnableDSP(bool enable = true);

    void RemovePreDSP(Uuid const& id);

    void RemovePostDSP(Uuid const& id);

    void SetEq(uint32_t band, float gain, float Q);

    void SetEq(EQSettings const& settings);

    void SetPreamp(float preamp);

    void SetReplayGain(float volume);

    bool IsEnableDSP() const;

    bool IsEnableSampleRateConverter() const;

    bool CanProcessFile() const noexcept;

    void Flush();

private:
    bool ApplyDSP(const float* samples, uint32_t num_samples, AudioBuffer<int8_t>& fifo);

    template <typename TDSP>
    TDSP* GetPreDSP() const {
        auto itr = std::find_if(pre_dsp_.begin(),
            pre_dsp_.end(),
            [](auto const& processor) {
                return processor->GetTypeId() == TDSP::Id;
            });
        if (itr == pre_dsp_.end()) {
            return nullptr;
        }
        return dynamic_cast<TDSP*>((*itr).get());
    }

    template <typename TDSP>
    TDSP* GetPostDSP() const {
        auto itr = std::find_if(post_dsp_.begin(),
            post_dsp_.end(),
            [](auto const& processor) {
                return processor->GetTypeId() == TDSP::Id;
            });
        if (itr == post_dsp_.end()) {
            return nullptr;
        }
        return dynamic_cast<TDSP*>((*itr).get());
    }

    bool enable_processor_;
    float replay_gain_;
    DsdModes dsd_modes_;
    EQSettings eq_settings_;
    std::vector<AlignPtr<IAudioProcessor>> setting_pre_dsp_chain_;
    std::vector<AlignPtr<IAudioProcessor>> setting_post_dsp_chain_;
    std::vector<AlignPtr<IAudioProcessor>> pre_dsp_;
    std::vector<AlignPtr<IAudioProcessor>> post_dsp_;
    AlignPtr<ISampleRateConverter> converter_;
    Buffer<float> dsp_buffer_;
    std::shared_ptr<spdlog::logger> logger_;
};

}
