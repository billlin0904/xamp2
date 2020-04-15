#include <vector>
#include <cassert>

#include <soxr.h>

#include <base/dll.h>
#include <base/logger.h>
#include <player/soxresampler.h>

namespace xamp::player {

class SoxrLib final {
public:
    SoxrLib() try
#ifdef XAMP_OS_WIN
        : module_(LoadDll("libsoxr.dll"))
#else
        : module_(LoadDll("libsoxr.dylib"))
#endif
        , soxr_quality_spec(module_, "soxr_quality_spec")
        , soxr_create(module_, "soxr_create")
        , soxr_process(module_, "soxr_process")
        , soxr_delete(module_, "soxr_delete")
        , soxr_io_spec(module_, "soxr_io_spec")
        , soxr_runtime_spec(module_, "soxr_runtime_spec") {
    }
    catch (const Exception & e) {
        XAMP_LOG_ERROR("{}", e.GetErrorMessage());
    }

    static XAMP_ALWAYS_INLINE SoxrLib& Instance() {
        static SoxrLib lib;
        return lib;
    }

    XAMP_DISABLE_COPY(SoxrLib)

private:
    ModuleHandle module_;

public:
    XAMP_DLL_HELPER(soxr_quality_spec) soxr_quality_spec;
    XAMP_DLL_HELPER(soxr_create) soxr_create;
    XAMP_DLL_HELPER(soxr_process) soxr_process;
    XAMP_DLL_HELPER(soxr_delete) soxr_delete;
    XAMP_DLL_HELPER(soxr_io_spec) soxr_io_spec;
    XAMP_DLL_HELPER(soxr_runtime_spec) soxr_runtime_spec;
};

struct SoxrHandleTraits final {
    static soxr_t invalid() noexcept {
        return nullptr;
    }

    static void close(soxr_t value) noexcept {
        SoxrLib::Instance().soxr_delete(value);
    }
};

using SoxrHandle = UniqueHandle<soxr_t, SoxrHandleTraits>;

class SoxrResampler::SoxrResamplerImpl {
public:
    SoxrResamplerImpl() noexcept
        : enable_steep_filter_(false)
        , quality_(SoxrQuality::LOW)
        , phase_(SoxrPhaseResponse::LINEAR_PHASE)
        , input_samplerate_(0)
        , num_channels_(0)
        , ratio_(0)
        , passband_(1.0)
        , stopband_(1.0) {
    }

    ~SoxrResamplerImpl() {
    }

    void Start(uint32_t input_samplerate, uint32_t num_channels, uint32_t output_samplerate) {
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

        if (enable_steep_filter_) {
            quality_spec |= SOXR_STEEP_FILTER;
        }

        quality_spec |= (SOXR_ROLLOFF_NONE | SOXR_HI_PREC_CLOCK | SOXR_VR);
        auto soxr_quality = SoxrLib::Instance().soxr_quality_spec(quality_spec, 0);

        switch (phase_) {
        case SoxrPhaseResponse::LINEAR_PHASE:
            soxr_quality.phase_response = SOXR_LINEAR_PHASE;
            break;
        case SoxrPhaseResponse::INTERMEDIATE_PHASE:
            soxr_quality.phase_response = SOXR_INTERMEDIATE_PHASE;
            break;
        case SoxrPhaseResponse::MINIMUM_PHASE:
            soxr_quality.phase_response = SOXR_MINIMUM_PHASE;
            break;
        }

        soxr_quality.passband_end = passband_;
        soxr_quality.stopband_begin = stopband_;

        auto iospec = SoxrLib::Instance().soxr_io_spec(SOXR_FLOAT32_I, SOXR_FLOAT32_I);
        auto runtimespec = SoxrLib::Instance().soxr_runtime_spec(1);

        soxr_error_t error = nullptr;
        handle_.reset(SoxrLib::Instance().soxr_create(input_samplerate,
                                                      output_samplerate,
                                                      num_channels,
                                                      &error,
                                                      &iospec,
                                                      &soxr_quality,
                                                      &runtimespec));
        if (!handle_) {
            XAMP_LOG_DEBUG("soxr error: {}", !error ? "" : error);
            throw Exception(Errors::XAMP_ERROR_LIBRARY_SPEC_ERROR,
                            "sox_create return failure!");
        }
        input_samplerate_ = input_samplerate;
        num_channels_ = num_channels;

        ratio_ = double(output_samplerate) / double(input_samplerate_);

        XAMP_LOG_ERROR("Soxr resampler setting=> {} {}", to_string(quality_), to_string(phase_));
    }

    void Close() {
        handle_.reset();
        buffer_.clear();
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

    bool Process(const float* samples, uint32_t num_sample, AudioBuffer<int8_t>& buffer) {
        assert(num_channels_ != 0);

        buffer_.resize(static_cast<size_t>(num_sample * ratio_) + 256);

        size_t samples_done = 0;

        SoxrLib::Instance().soxr_process(handle_.get(),
                                         samples,
                                         num_sample / num_channels_,
                                         nullptr,
                                         buffer_.data(),
                                         buffer_.size() / num_channels_,
                                         &samples_done);

        if (!samples_done) {
            return false;
        }

        uint32_t write_size(samples_done * num_channels_ * sizeof(float));
        buffer.TryWrite(reinterpret_cast<const int8_t*>(buffer_.data()), write_size);
        buffer_.resize(samples_done * num_channels_);
        return true;
    }

    bool enable_steep_filter_;
    SoxrQuality quality_;
    SoxrPhaseResponse phase_;
    uint32_t input_samplerate_;
    uint32_t num_channels_;
    double ratio_;
    double passband_;
    double stopband_;
    SoxrHandle handle_;
    std::vector<float> buffer_;
};

SoxrResampler::SoxrResampler()
    : impl_(MakeAlign<SoxrResamplerImpl>()) {
}

SoxrResampler::~SoxrResampler() = default;

void SoxrResampler::LoadSoxrLib() {
    SoxrLib::Instance();
}

void SoxrResampler::Start(uint32_t input_samplerate, uint32_t num_channels, uint32_t output_samplerate, uint32_t) {
    impl_->Start(input_samplerate, num_channels, output_samplerate);
}

void SoxrResampler::SetSteepFilter(bool enable) {
    impl_->SetSteepFilter(enable);
}

void SoxrResampler::SetQuality(SoxrQuality quality) {
    impl_->SetQuality(quality);
}

void SoxrResampler::SetPhase(SoxrPhaseResponse phase) {
    impl_->SetPhase(phase);
}

void SoxrResampler::SetPassBand(double passband) {
    impl_->SetPassBand(passband);
}

void SoxrResampler::SetStopBand(double stopband) {
    impl_->SetStopBand(stopband);
}

std::string_view SoxrResampler::GetDescription() const noexcept {
    return "Soxr";
}

bool SoxrResampler::Process(const float* samples, uint32_t num_sample, AudioBuffer<int8_t>& buffer) {
    return impl_->Process(samples, num_sample, buffer);
}

}

