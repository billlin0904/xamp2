#include <base/assert.h>
#include <base/audioformat.h>
#include <base/exception.h>

#include <stream/iequalizer.h>
#include <stream/soxresampler.h>
#include <stream/bassvolume.h>
#include <stream/passthroughsamplerateconverter.h>
#include <stream/dspmanager.h>

namespace xamp::stream {

static void AddOrReplace(AlignPtr<IAudioProcessor> processor, std::vector<AlignPtr<IAudioProcessor>>& dsp_chain) {
    auto id = processor->GetTypeId();
    const auto itr = std::find_if(dsp_chain.begin(),
        dsp_chain.end(),
        [id](auto const& processor) {
            return processor->GetTypeId() == id;
        });
    XAMP_LOG_DEBUG("Add dsp:{} success.", processor->GetDescription());
    if (itr != dsp_chain.end()) {
        *itr = std::move(processor);
    }
    else {
        dsp_chain.push_back(std::move(processor));
    }
}

static void Remove(Uuid const& id, std::vector<AlignPtr<IAudioProcessor>>& dsp_chain) {
    auto itr = std::remove_if(dsp_chain.begin(),
        dsp_chain.end(),
        [id](auto const& processor) {
            return processor->GetTypeId() == id;
        });
    if (itr != dsp_chain.end()) {
        XAMP_LOG_DEBUG("Remove post dsp:{} success.", (*itr)->GetDescription());
    }
}

DSPManager::DSPManager()
	: enable_processor_(false)
	, replay_gain_(0.0)
	, dsd_modes_(DsdModes::DSD_MODE_PCM) {
    logger_ = Logger::GetInstance().GetLogger("DSPManager");
}

void DSPManager::AddPostDSP(AlignPtr<IAudioProcessor> processor) {
    AddOrReplace(std::move(processor), post_dsp_);
    EnableDSP();
}

void DSPManager::AddPreDSP(AlignPtr<IAudioProcessor> processor) {
    AddOrReplace(std::move(processor), pre_dsp_);
    EnableDSP();
}

void DSPManager::EnableDSP(bool enable) {
    enable_processor_ = enable;
    XAMP_LOG_D(logger_, "Enable processor {}", enable);
}

void DSPManager::RemovePostDSP(Uuid const& id) {
    Remove(id, post_dsp_);
    if (post_dsp_.empty()) {
        EnableDSP(false);
    }
}

void DSPManager::RemovePreDSP(Uuid const& id) {
    Remove(id, pre_dsp_);
    if (pre_dsp_.empty()) {
        EnableDSP(false);
    }
}

void DSPManager::SetEq(EQSettings const& settings) {
    eq_settings_ = settings;
}

void DSPManager::SetEq(uint32_t band, float gain, float Q) {
    eq_settings_.bands[band].gain = gain;
    eq_settings_.bands[band].Q = Q;
}

void DSPManager::SetPreamp(float preamp) {
    eq_settings_.preamp = preamp;
}

void DSPManager::SetReplayGain(double volume) {
    replay_gain_ = volume;
}

bool DSPManager::IsEnableDSP() const noexcept {
    return enable_processor_;
}

bool DSPManager::CanProcessFile() const noexcept {
    return (dsd_modes_ == DsdModes::DSD_MODE_PCM || dsd_modes_ == DsdModes::DSD_MODE_DSD2PCM) && !pre_dsp_.empty() && IsEnableDSP();
}

bool DSPManager::IsEnableSampleRateConverter() const {
    return GetPreDSP<SoxrSampleRateConverter>() != std::nullopt
        || GetPostDSP<SoxrSampleRateConverter>() != std::nullopt;
}

void DSPManager::Flush() {
    for (const auto& pre_dsp : pre_dsp_) {
        pre_dsp->Flush();
    }
    for (const auto& post_dsp : post_dsp_) {
        post_dsp->Flush();
    }
}

bool DSPManager::ProcessDSP(const float* samples, uint32_t num_samples, AudioBuffer<int8_t>& fifo) {
    if (CanProcessFile()) {
        return ApplyDSP(samples, num_samples, fifo);
    }
    BufferOverFlowThrow(converter_->Process(samples, num_samples, fifo));
    return false;
}

bool DSPManager::ApplyDSP(const float* samples, uint32_t num_samples, AudioBuffer<int8_t>& fifo) {
    if (!pre_dsp_.empty()) {
        temp_.resize(num_samples);

        for (const auto& pre_dsp : pre_dsp_) {
            if (!pre_dsp->Process(samples, num_samples, temp_)) {
                return true;
            }
        }

        for (const auto& post_dsp : post_dsp_) {
            post_dsp->Process(temp_.data(), temp_.size(), dsp_buffer_);
        }
    } else {
        for (const auto& post_dsp : post_dsp_) {
            post_dsp->Process(samples, num_samples, dsp_buffer_);
        }
    }

    BufferOverFlowThrow(converter_->Process(dsp_buffer_, fifo));
    return false; 
}

void DSPManager::Init(AudioFormat input_format, AudioFormat output_format, DsdModes dsd_mode, uint32_t sample_size) {
    converter_ = MakeAlign<ISampleRateConverter, PassThroughSampleRateConverter>(dsd_mode, sample_size);
    dsd_modes_ = dsd_mode;

    if (!enable_processor_) {
        return;
    }

    for (const auto& dsp : pre_dsp_) {
        dsp->Start(output_format.GetSampleRate());
        XAMP_LOG_D(logger_, "Init {} .", dsp->GetDescription());
    }

    for (const auto& dsp : post_dsp_) {
        dsp->Start(output_format.GetSampleRate());
        XAMP_LOG_D(logger_, "Init {} .", dsp->GetDescription());
    }

    if (const auto converter = GetPreDSP<SoxrSampleRateConverter>()) {
        converter.value()->Init(input_format.GetSampleRate());
    }

    if (const auto volume = GetPostDSP<BassVolume>()) {
        volume.value()->Init(replay_gain_);
        XAMP_LOG_D(logger_, "Set replay gain: {} db.", replay_gain_);
    }

    if (const auto eq = GetPostDSP<IEqualizer>()) {
        eq.value()->SetEQ(eq_settings_);
        int i = 0;
        for (auto [gain, Q] : eq_settings_.bands) {
            XAMP_LOG_D(logger_, "Set EQ band: {} gain: {} Q: {}.", i++, gain, Q);
        }
        XAMP_LOG_D(logger_, "Set preamp: {}.", eq_settings_.preamp);
    }
}

}
