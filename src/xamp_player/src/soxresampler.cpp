#include <vector>
#include <cassert>

#include <soxr.h>

#include <base/dataconverter.h>
#include <base/singleton.h>
#include <base/dll.h>
#include <base/logger.h>
#include <base/vmmemlock.h>
#include <player/soxresampler.h>

namespace xamp::player {

class SoxrLib final {
public:
    friend class Singleton<SoxrLib>;

    XAMP_DISABLE_COPY(SoxrLib)

private:
    SoxrLib() try
        : module_(LoadModule(GetDllFileName("libsoxr")))
        , soxr_quality_spec(module_, "soxr_quality_spec")
        , soxr_create(module_, "soxr_create")
        , soxr_process(module_, "soxr_process")
        , soxr_delete(module_, "soxr_delete")
        , soxr_io_spec(module_, "soxr_io_spec")
        , soxr_runtime_spec(module_, "soxr_runtime_spec")
        , soxr_clear(module_, "soxr_clear") {
    }
    catch (const Exception& e) {
        XAMP_LOG_ERROR("{}", e.GetErrorMessage());
    }

    ModuleHandle module_;

public:
    XAMP_DECLARE_DLL(soxr_quality_spec) soxr_quality_spec;
    XAMP_DECLARE_DLL(soxr_create) soxr_create;
    XAMP_DECLARE_DLL(soxr_process) soxr_process;
    XAMP_DECLARE_DLL(soxr_delete) soxr_delete;
    XAMP_DECLARE_DLL(soxr_io_spec) soxr_io_spec;
    XAMP_DECLARE_DLL(soxr_runtime_spec) soxr_runtime_spec;
    XAMP_DECLARE_DLL(soxr_clear) soxr_clear;
};

class SoxrSampleRateConverter::SoxrResamplerImpl final {
public:
    static constexpr size_t kInitBufferSize = 4 * 1024 * 1204;

    SoxrResamplerImpl() noexcept
        : enable_steep_filter_(false)
        , enable_dither_(false)
        , quality_(SoxrQuality::VHQ)
        , phase_(SoxrPhaseResponse::LINEAR_PHASE)
        , input_samplerate_(0)
        , num_channels_(0)
        , ratio_(0)
        , passband_(0.997)
        , stopband_(1.0) {        
    }

    ~SoxrResamplerImpl() noexcept {
        Close();
    }

    void Start(uint32_t input_sample_rate, uint32_t num_channels, uint32_t output_sample_rate) {        
        Close();

        unsigned long quality_spec = 0;

        switch (quality_) {
        case SoxrQuality::UHQ:
            quality_spec |= SOXR_32_BITQ;
            break;
        case SoxrQuality::VHQ:
            quality_spec |= SOXR_VHQ;
            break;
        case SoxrQuality::HQ:
            quality_spec |= SOXR_HQ;
            break;
        case SoxrQuality::MQ:
            quality_spec |= SOXR_MQ;
            break;
        case SoxrQuality::LOW:
            quality_spec |= SOXR_LQ;
            break;
		}

        switch (phase_) {
        case SoxrPhaseResponse::LINEAR_PHASE:
            quality_spec |= SOXR_LINEAR_PHASE;
            break;
        case SoxrPhaseResponse::INTERMEDIATE_PHASE:
            quality_spec |= SOXR_INTERMEDIATE_PHASE;
            break;
        case SoxrPhaseResponse::MINIMUM_PHASE:
            quality_spec |= SOXR_MINIMUM_PHASE;
            break;
        }

        auto flags = (SOXR_ROLLOFF_NONE | SOXR_HI_PREC_CLOCK | SOXR_VR | SOXR_DOUBLE_PRECISION);
        if (enable_steep_filter_) {
            flags |= SOXR_STEEP_FILTER;
        }

        auto soxr_quality = Singleton<SoxrLib>::GetInstance().soxr_quality_spec(quality_spec, flags);

        soxr_quality.passband_end = passband_;
        soxr_quality.stopband_begin = stopband_;

        auto iospec = Singleton<SoxrLib>::GetInstance().soxr_io_spec(SOXR_FLOAT32_I, SOXR_FLOAT32_I);

        if (!enable_dither_) {
            iospec.flags |= SOXR_NO_DITHER;
        }

        auto runtimespec = Singleton<SoxrLib>::GetInstance().soxr_runtime_spec(1);

        soxr_error_t error = nullptr;
        handle_.reset(Singleton<SoxrLib>::GetInstance().soxr_create(input_sample_rate,
                                                      output_sample_rate,
                                                      num_channels,
                                                      &error,
                                                      &iospec,
                                                      &soxr_quality,
                                                      &runtimespec));
        if (!handle_) {
            XAMP_LOG_DEBUG("soxr error: {}", !error ? "" : error);
            throw LibrarySpecException("sox_create return failure!");
        }

        input_samplerate_ = input_sample_rate;
        num_channels_ = num_channels;

        ratio_ = static_cast<double>(output_sample_rate) / static_cast<double>(input_samplerate_);

        XAMP_LOG_DEBUG("Soxr resampler setting=> input:{} output:{} quality:{} phase:{} pass:{} stopband:{}",
            input_sample_rate,
            output_sample_rate,
            EnumToString(quality_),
            EnumToString(phase_),
            passband_,
            stopband_);

        ResizeBuffer(kInitBufferSize);        
    }

    void Close() noexcept {
        vmlock_.UnLock();
        handle_.reset();
        buffer_.clear();
    }

    void SetDither(bool enable) {
        enable_dither_ = enable;
    }

    void SetSteepFilter(bool enable) {
        enable_steep_filter_ = enable;
    }

    void SetQuality(SoxrQuality quality) {
        quality_ = quality;
    }

    void SetPhase(SoxrPhaseResponse phase) {
        phase_ = phase;
    }

    void SetPassBand(double passband) {
        passband_ = passband;
    }

    void SetStopBand(double stopband) {
        stopband_ = stopband;
    }

    void Flush() {
        if (!handle_) {
            return;
        }
        Singleton<SoxrLib>::GetInstance().soxr_clear(handle_.get());
    }

    bool Process(float const * samples, uint32_t num_sample, AudioBuffer<int8_t>& buffer) {
        assert(num_channels_ != 0);

        auto required_size = static_cast<size_t>(num_sample * ratio_) + 256;
        if (required_size > buffer_.size()) {
            ResizeBuffer(required_size);
        }        

        size_t samples_done = 0;

        Singleton<SoxrLib>::GetInstance().soxr_process(handle_.get(),
                                         samples,
                                         num_sample / num_channels_,
                                         nullptr,
                                         buffer_.data(),
                                         buffer_.size() / num_channels_,
                                         &samples_done);

        if (!samples_done) {
            return false;
        }

        const auto write_size(samples_done * num_channels_ * sizeof(float));

    	// Note: libsoxr 並不會將sample進行限制大小.
        ClampSample(buffer_.data(), samples_done * num_channels_);
    	
        CheckBufferFlow(buffer.TryWrite(reinterpret_cast<int8_t const*>(buffer_.data()), write_size));

        required_size = samples_done * num_channels_;
        if (required_size > buffer_.size()) {
            ResizeBuffer(required_size);
        }        
        return true;
    }

    void ResizeBuffer(size_t required_size) {
        vmlock_.UnLock();
        buffer_.resize(required_size);
        vmlock_.Lock(buffer_.data(), required_size * sizeof(float));
    }

    struct SoxrHandleTraits final {
        static soxr_t invalid() noexcept {
            return nullptr;
        }

        static void close(soxr_t value) noexcept {
            Singleton<SoxrLib>::GetInstance().soxr_delete(value);
        }
    };

    using SoxrHandle = UniqueHandle<soxr_t, SoxrHandleTraits>;

    bool enable_steep_filter_;
    bool enable_dither_;
    SoxrQuality quality_;
    SoxrPhaseResponse phase_;
    uint32_t input_samplerate_;
    uint32_t num_channels_;
    double ratio_;
    double passband_;
    double stopband_;
    SoxrHandle handle_;
    VmMemLock vmlock_;
    Buffer<float> buffer_;
};

const std::string_view SoxrSampleRateConverter::VERSION = "Soxr " SOXR_THIS_VERSION_STR;

SoxrSampleRateConverter::SoxrSampleRateConverter()
    : impl_(MakeAlign<SoxrResamplerImpl>()) {
}

XAMP_PIMPL_IMPL(SoxrSampleRateConverter)
	
void SoxrSampleRateConverter::LoadSoxrLib() {
    (void)Singleton<SoxrLib>::GetInstance();
}

void SoxrSampleRateConverter::Start(uint32_t input_samplerate, uint32_t num_channels, uint32_t output_samplerate) {
    impl_->Start(input_samplerate, num_channels, output_samplerate);
}

void SoxrSampleRateConverter::SetSteepFilter(bool enable) {
    impl_->SetSteepFilter(enable);
}

void SoxrSampleRateConverter::SetQuality(SoxrQuality quality) {
    impl_->SetQuality(quality);
}

void SoxrSampleRateConverter::SetPhase(SoxrPhaseResponse phase) {
    impl_->SetPhase(phase);
}

void SoxrSampleRateConverter::SetPassBand(double passband) {
    impl_->SetPassBand(passband);
}

void SoxrSampleRateConverter::SetStopBand(double stopband) {
    impl_->SetStopBand(stopband);
}

void SoxrSampleRateConverter::SetDither(bool enable) {
    impl_->SetDither(enable);
}

std::string_view SoxrSampleRateConverter::GetDescription() const noexcept {
    return VERSION;
}

bool SoxrSampleRateConverter::Process(float const * samples, uint32_t num_sample, AudioBuffer<int8_t>& buffer) {
    return impl_->Process(samples, num_sample, buffer);
}

void SoxrSampleRateConverter::Flush() {
    impl_->Flush();
}

AlignPtr<SampleRateConverter> SoxrSampleRateConverter::Clone() {
    auto other = MakeAlign<SampleRateConverter, SoxrSampleRateConverter>();
    auto* converter = reinterpret_cast<SoxrSampleRateConverter*>(other.get());
    converter->SetQuality(impl_->quality_);
    converter->SetPassBand(impl_->passband_);
    converter->SetPhase(impl_->phase_);
    converter->SetStopBand(impl_->stopband_);
    converter->SetSteepFilter(impl_->enable_steep_filter_);
    converter->SetDither(impl_->enable_dither_);
    return other;
}

}

