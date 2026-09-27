#include <stream/dspmanager.h>

#include <stream/api.h>
#include <stream/soxresampler.h>
#include <stream/srcresampler.h>
#include <stream/dsdmodesamplewriter.h>
#include <stream/bassparametriceq.h>

#ifdef XAMP_OS_WIN
#include <stream/r8brainresampler.h>
#endif

#include <base/exception.h>
#include <base/logger.h>
#include <base/stl.h>

XAMP_STREAM_NAMESPACE_BEGIN

namespace {
    XAMP_DECLARE_LOG_NAME(DspManager);
    inline constexpr int32_t kDefaultBufSize = 1024 * 1024;
}

DSPManager::DSPManager() {
    logger_ = XAMP_LOG_CREATE_LOGGER(DspManager);
    pre_dsp_buffer_.resize(kDefaultBufSize);
    post_dsp_buffer_.resize(kDefaultBufSize);
}

void DSPManager::addPostDSP(ScopedPtr<IAudioProcessor> processor) {
    XAMP_LOG_D(logger_, "Add post dsp:{} success.", processor->getDescription());
    addOrReplace(std::move(processor), post_dsp_);
}

void DSPManager::addPreDSP(ScopedPtr<IAudioProcessor> processor) {
    XAMP_LOG_D(logger_, "Add pre dsp:{} success.", processor->getDescription());
    addOrReplace(std::move(processor), pre_dsp_);
}

IDSPManager& DSPManager::addParametricEq() {
    addPostDSP(StreamFactory::makeParametricEq());
    return *this;
}

IDSPManager& DSPManager::setParametricEq(bool enabled, const EqSettings& settings, const Property& config) {
    if (bitperfect_) return *this;
    auto next_config = config;
    const auto update_dispatch = [this]() {
        if (!canProcess()) {
            dispatch_ = bind_front(&DSPManager::defaultProcess, this);
        }
        else {
            dispatch_ = bind_front(&DSPManager::process, this);
        }
    };

    if (!enabled) {
        removePostDSP<BassParametricEq>();
        config_ = std::move(next_config);
        update_dispatch();
        return *this;
    }

    next_config.create(DspConfig::kEQSettings, settings);
    if (const auto parametric_eq = getPostDSP<BassParametricEq>()) {
        if (*parametric_eq != nullptr) {
            (*parametric_eq)->setEq(settings);
            config_ = std::move(next_config);
            update_dispatch();
            return *this;
        }
    }

    auto processor = StreamFactory::makeParametricEq();
    processor->initialize(next_config);
    addPostDSP(std::move(processor));
    config_ = std::move(next_config);
    update_dispatch();
    return *this;
}

void DSPManager::setSampleWriter(ScopedPtr<ISampleWriter> writer) {
    sample_writer_ = std::move(writer);
}

IDSPManager& DSPManager::removeParametricEq() {
    removePostDSP<BassParametricEq>();
    return *this;
}

IDSPManager& DSPManager::removeSampleRateConverter() {
#ifdef XAMP_OS_WIN
    removePreDSP<R8brainSampleRateConverter>();
#endif
    removePreDSP<SoxrSampleRateConverter>();
    removePreDSP<SrcSampleRateConverter>();
    return *this;
}

bool DSPManager::canProcess() const {
    if (bitperfect_) return false;
    if (pre_dsp_.empty() && post_dsp_.empty()) {
        return false;
    }

	const auto dsd_mode = config_.get<DsdModes>(DspConfig::kDsdMode);
    if (dsd_mode == DsdModes::DSD_MODE_PCM
        || dsd_mode == DsdModes::DSD_MODE_DSD2PCM) {
        return true;
    }
    return false;
}

bool DSPManager::contains(const Uuid& type) const {
    return contains([type](const auto& id) {
        return id == type;
        });
}

void DSPManager::addOrReplace(ScopedPtr<IAudioProcessor> processor, std::vector<ScopedPtr<IAudioProcessor>>& dsp_chain) {
    auto id = processor->getTypeId();
    const auto itr = std::find_if(dsp_chain.begin(), dsp_chain.end(),
                                          [id](auto const& processor) {
	                                          return processor->getTypeId() == id;
                                          });
    if (itr != dsp_chain.end()) {
        *itr = std::move(processor);
    }
    else {
        dsp_chain.push_back(std::move(processor));
    }
}

bool DSPManager::isEnableSampleRateConverter() const {
    if (bitperfect_) return false;
    const auto equal_id = [](const auto& id) {
        return XAMP_UUID_OF(SoxrSampleRateConverter) == id
#ifdef XAMP_OS_WIN
        || XAMP_UUID_OF(R8brainSampleRateConverter) == id
#endif
    	|| XAMP_UUID_OF(SrcSampleRateConverter) == id;
    };
    return contains(equal_id);
}

bool DSPManager::processDSP(const float* samples, uint32_t num_samples, AudioBuffer<std::byte>& fifo) {
    if (bitperfect_) throw std::runtime_error("BitPerfect requires typed integer PCM bytes");
    return std::invoke(dispatch_, samples, num_samples, fifo);
}

void DSPManager::processPcm(const xamp::pcm::Block& block, AudioBuffer<std::byte>& fifo) {
    if (!bitperfect_ || !xamp::pcm::valid(block.format) ||
        block.format.layout != xamp::pcm::Layout::Interleaved ||
        block.frames > SIZE_MAX / block.format.frameBytes() ||
        block.data.size() != block.frames * block.format.frameBytes())
        throw std::runtime_error("Invalid strict PCM block");
    failWith<BufferOverflowException>(fifo.tryWrite(block.data.data(), block.data.size()),
        "BitPerfect FIFO overflow");
}

bool DSPManager::defaultProcess(const float* samples, uint32_t num_samples, AudioBuffer<std::byte>& fifo) {
    failWith<BufferOverflowException>(sample_writer_->process(samples, num_samples, fifo),
        "Failed to write buffer, read:{} write:{}",
        fifo.getAvailableRead(),
        fifo.getAvailableWrite());
    return false;
}

bool DSPManager::process(const float* samples, uint32_t num_samples, AudioBuffer<std::byte>& fifo) {
    BufferRef<float> pre_dsp_buffer(pre_dsp_buffer_);
    BufferRef<float> post_dsp_buffer(post_dsp_buffer_);

    const float* input = samples;
    size_t count = num_samples;
    bool use_pre = true;
    const auto run = [&](const auto& chain) {
        for (const auto& processor : chain) {
            auto& output = use_pre ? pre_dsp_buffer : post_dsp_buffer;
            if (!processor->process(input, count, output)) return false;
            input = output.data();
            count = output.size();
            use_pre = !use_pre;
        }
        return true;
    };
    if (!run(pre_dsp_) || !run(post_dsp_)) return true;
    failWith<BufferOverflowException>(sample_writer_->process(input, count, fifo),
        "Failed to write DSP buffer, read:{} write:{}", fifo.getAvailableRead(), fifo.getAvailableWrite());
    return false;
}

void DSPManager::initialize(const Property& config) {
    config_ = config;
    if (bitperfect_) return;

    if (!sample_writer_) {
        auto sample_size = config_.get<uint32_t>(DspConfig::kSampleSize);
        auto dsd_mode = config_.get<DsdModes>(DspConfig::kDsdMode);
        sample_writer_ = makeAlign<ISampleWriter, DsdModeSampleWriter>(dsd_mode, sample_size);
    }

    if (!canProcess()) {
        dispatch_ = bind_front(&DSPManager::defaultProcess, this);
        return;
    }
    else {
        dispatch_ = bind_front(&DSPManager::process, this);
    }

    for (const auto& dsp : pre_dsp_) {
        dsp->initialize(config_);
    }

    for (const auto& dsp : post_dsp_) {
        dsp->initialize(config_);
    }
}

XAMP_STREAM_NAMESPACE_END
