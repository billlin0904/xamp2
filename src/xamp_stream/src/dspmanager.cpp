#include <stream/dspmanager.h>

#include <stream/api.h>
#include <stream/bassequalizer.h>
#include <stream/soxresampler.h>
#include <stream/r8brainresampler.h>
#include <stream/bassvolume.h>
#include <stream/dsdmodesamplewriter.h>
#include <stream/basscompressor.h>

#include <base/exception.h>
#include <base/logger_impl.h>

XAMP_STREAM_NAMESPACE_BEGIN

XAMP_DECLARE_LOG_NAME(DspManager);

static constexpr int32_t kDefaultBufSize = 1024 * 1024;

DSPManager::DSPManager() {
    logger_ = LoggerManager::GetInstance().GetLogger(kDspManagerLoggerName);
    pre_dsp_buffer_.resize(kDefaultBufSize);
    post_dsp_buffer_.resize(kDefaultBufSize);
}

void DSPManager::AddPostDSP(AlignPtr<IAudioProcessor> processor) {
    XAMP_LOG_D(logger_, "Add post dsp:{} success.", processor->GetDescription());
    AddOrReplace(std::move(processor), post_dsp_);
}

void DSPManager::AddPreDSP(AlignPtr<IAudioProcessor> processor) {
    XAMP_LOG_D(logger_, "Add pre dsp:{} success.", processor->GetDescription());
    AddOrReplace(std::move(processor), pre_dsp_);
}

IDSPManager& DSPManager::AddEqualizer() {
    AddPostDSP(StreamFactory::MakeEqualizer());
    return *this;
}

IDSPManager& DSPManager::RemoveCompressor() {
    RemovePostDSP<BassCompressor>();
    return *this;
}

IDSPManager& DSPManager::AddCompressor() {
    AddPostDSP(StreamFactory::MakeCompressor());
    return *this;
}

IDSPManager& DSPManager::AddVolumeControl() {
    AddPostDSP(StreamFactory::MakeVolume());
    return *this;
}

void DSPManager::SetSampleWriter(AlignPtr<ISampleWriter> writer) {
    if (!writer) {
        sample_writer_.reset();
        return;
    }
    sample_writer_ = std::move(writer);
}

IDSPManager& DSPManager::RemoveEqualizer() {
    RemovePostDSP<BassEqualizer>();
    return *this;
}

IDSPManager& DSPManager::RemoveVolumeControl() {
    RemovePreDSP<BassVolume>();
    return *this;
}

IDSPManager& DSPManager::RemoveSampleRateConverter() {
    RemovePostDSP<R8brainSampleRateConverter>();
    RemovePostDSP<SoxrSampleRateConverter>();
    return *this;
}

bool DSPManager::CanProcess() const noexcept {
    if (pre_dsp_.empty() && post_dsp_.empty()) {
        return false;
    }

	const auto dsd_mode = config_.Get<DsdModes>(DspConfig::kDsdMode);
    if (dsd_mode == DsdModes::DSD_MODE_PCM
        || dsd_mode == DsdModes::DSD_MODE_DSD2PCM) {
        return true;
    }
    return false;
}

void DSPManager::AddOrReplace(AlignPtr<IAudioProcessor> processor, Vector<AlignPtr<IAudioProcessor>>& dsp_chain) {
    auto id = processor->GetTypeId();
    const auto itr = std::find_if(dsp_chain.begin(),
        dsp_chain.end(),
        [id](auto const& processor) {
            return processor->GetTypeId() == id;
        });
    if (itr != dsp_chain.end()) {
        *itr = std::move(processor);
    }
    else {
        dsp_chain.push_back(std::move(processor));
    }
}

bool DSPManager::IsEnableSampleRateConverter() const {
    const auto equal_id = [](const auto& id) {
        return XAMP_UUID_OF(SoxrSampleRateConverter) == id || XAMP_UUID_OF(R8brainSampleRateConverter) == id;
    };
    return Contains(equal_id);
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
    return dispatch_(samples, num_samples, fifo);
}

bool DSPManager::DefaultProcess(const float* samples, uint32_t num_samples, AudioBuffer<int8_t>& fifo) {
    ThrowIf<BufferOverflowException>(sample_writer_->Process(samples, num_samples, fifo), 
        "Failed to write buffer, read:{} write:{}", 
        fifo.GetAvailableRead(),
        fifo.GetAvailableWrite());
    return false;
}

bool DSPManager::Process(const float* samples, uint32_t num_samples, AudioBuffer<int8_t>& fifo) {
    BufferRef<float> pre_dsp_buffer(pre_dsp_buffer_);
    BufferRef<float> post_dsp_buffer(post_dsp_buffer_);

    if (!pre_dsp_.empty()) {
        for (const auto& pre_dsp : pre_dsp_) {
            if (!pre_dsp->Process(samples, num_samples, pre_dsp_buffer)) {
                return true;
            }
        }
        for (const auto& post_dsp : post_dsp_) {
            post_dsp->Process(pre_dsp_buffer.data(),
                pre_dsp_buffer.size(),
                post_dsp_buffer);
        }
    }
    else {
        for (const auto& post_dsp : post_dsp_) {
            post_dsp->Process(samples, num_samples, post_dsp_buffer);
        }
    }

    if (post_dsp_.empty()) {
        ThrowIf<BufferOverflowException>(sample_writer_->Process(pre_dsp_buffer, fifo),
            "Failed to write pre buffer, read:{} write:{}",
            fifo.GetAvailableRead(),
            fifo.GetAvailableWrite());
    }
    else {
        ThrowIf<BufferOverflowException>(sample_writer_->Process(post_dsp_buffer, fifo),
            "Failed to write post buffer, read:{} write:{}",
            fifo.GetAvailableRead(),
            fifo.GetAvailableWrite());
    }
    return false;
}

void DSPManager::Init(const AnyMap& config) {
    config_ = config;

    if (!sample_writer_) {
        auto sample_size = config_.Get<uint8_t>(DspConfig::kSampleSize);
        auto dsd_mode = config_.Get<DsdModes>(DspConfig::kDsdMode);
        sample_writer_ = MakeAlign<ISampleWriter, DsdModeSampleWriter>(dsd_mode, sample_size);
    }

    if (!CanProcess()) {
        dispatch_ = bind_front(&DSPManager::DefaultProcess, this);
        return;
    }
    else {        
        dispatch_ = bind_front(&DSPManager::Process, this);
    }

    for (const auto& dsp : pre_dsp_) {
        dsp->Start(config_);
        dsp->Init(config_);
    }

    for (const auto& dsp : post_dsp_) {
        dsp->Start(config_);
        dsp->Init(config_);
    }
}

XAMP_STREAM_NAMESPACE_END
