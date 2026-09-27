#include <base/logger.h>
#include <cmath>
#include <stdexcept>
#include <stream/basslib.h>
#include <stream/bass_util.h>
#include <stream/eqsettings.h>
#include <stream/bassparametriceq.h>

XAMP_STREAM_NAMESPACE_BEGIN

XAMP_DECLARE_LOG_NAME(BassParametricEq);

namespace {
    constexpr float kMinBqfBandwidth = 0.1F;
    constexpr float kMaxBqfBandwidth = 10.0F;

    float NormalizeBqfBandwidth(float bandwidth) noexcept {
        if (bandwidth >= kMinBqfBandwidth && bandwidth < kMaxBqfBandwidth) {
            return bandwidth;
        }
        return 0;
    }
}

class BassParametricEq::BassParametricEqImpl {
public:
    BassParametricEqImpl()
		: preamp_(0) {
	    logger_ = XAMP_LOG_CREATE_LOGGER(BassParametricEq);
    }
    
    void start(uint32_t sample_rate, uint32_t channels) {
        RemoveFx();

        impl_.reset(LIB_BASS.BASS_StreamCreate(sample_rate,
            channels,
            BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE,
            STREAMPROC_DUMMY,
            nullptr));
        BassIfFailedThrow(impl_);

        preamp_ = LIB_BASS.BASS_ChannelSetFX(impl_.get(), BASS_FX_BFX_VOLUME, 0);
        BassIfFailedThrow(preamp_);

        sample_rate_ = sample_rate;
        channels_ = channels;
    }

    void AddBand(EQFilterTypes filter, float fCenter, float fBandWidth, float fGain, float fQ, float fS) {
        if (!std::isfinite(fCenter) || fCenter < 1 || fCenter >= sample_rate_ / 2.0f ||
            !std::isfinite(fGain) || !std::isfinite(fQ) || fQ < 0 ||
            !std::isfinite(fBandWidth) || fBandWidth < 0 ||
            (fBandWidth != 0 && NormalizeBqfBandwidth(fBandWidth) == 0) ||
            !std::isfinite(fS) || fS < 0)
            throw std::invalid_argument("Invalid EQ band parameters");
        const bool shelf = filter == EQFilterTypes::FT_LOW_SHELF || filter == EQFilterTypes::FT_LOW_HIGH_SHELF;
        if (fS != 0 && !shelf) throw std::invalid_argument("Slope requires a shelf filter");
        if (shelf && fS == 0) {
            // BASS shelves use RBJ slope, while the editor exposes RBJ Q.
            const double a = std::pow(10.0, fGain / 40.0);
            const double omega = 2.0 * 3.141592653589793 * fCenter / sample_rate_;
            const double inverse_q_squared = fBandWidth > 0
                ? std::pow(2.0 * std::sinh(std::log(2.0) / 2.0 * fBandWidth * omega / std::sin(omega)), 2.0)
                : (fQ > 0 ? 1.0 / (fQ * fQ) : 0.0);
            const double denominator = 2.0 + (inverse_q_squared - 2.0) / (a + 1.0 / a);
            if (inverse_q_squared <= 0 || denominator <= 0)
                throw std::invalid_argument("Invalid shelf Q or bandwidth");
            fS = static_cast<float>(1.0 / denominator);
        }
        if (fS > 0) { fQ = 0; fBandWidth = 0; }
        else if (fBandWidth > 0) fQ = 0;
        else if (fQ == 0) throw std::invalid_argument("EQ requires Q, bandwidth or slope");
        const auto fx_handle = LIB_BASS.BASS_ChannelSetFX(impl_.get(), BASS_FX_BFX_BQF, 1);
        BassIfFailedThrow(fx_handle);

        BASS_BFX_BQF bqf{};

        switch (filter) {
        case EQFilterTypes::FT_UNKNOWN:
            bqf.lFilter = BASS_BFX_BQF_PEAKINGEQ;
            filter = EQFilterTypes::FT_ALL_PEAKING_EQ;
            break;
        case EQFilterTypes::FT_LOW_SHELF:
            bqf.lFilter = BASS_BFX_BQF_LOWSHELF;

            break;
        case EQFilterTypes::FT_LOW_HIGH_SHELF:
            bqf.lFilter = BASS_BFX_BQF_HIGHSHELF;

            break;
        case EQFilterTypes::FT_LOW_PASS:
            bqf.lFilter = BASS_BFX_BQF_LOWPASS;
            break;
        case EQFilterTypes::FT_HIGH_PASS:
            bqf.lFilter = BASS_BFX_BQF_HIGHPASS;
            break;
        case EQFilterTypes::FT_HIGH_BAND_PASS:
            bqf.lFilter = BASS_BFX_BQF_BANDPASS;
            break;
        case EQFilterTypes::FT_HIGH_BAND_PASS_Q:
            bqf.lFilter = BASS_BFX_BQF_BANDPASS_Q;
            break;
        case EQFilterTypes::FT_NOTCH:
            bqf.lFilter = BASS_BFX_BQF_NOTCH;
            break;
        case EQFilterTypes::FT_ALL_PASS:
            bqf.lFilter = BASS_BFX_BQF_ALLPASS;
            break;
        case EQFilterTypes::FT_ALL_PEAKING_EQ:
            bqf.lFilter = BASS_BFX_BQF_PEAKINGEQ;
            break;
        default:
            LIB_BASS.BASS_ChannelRemoveFX(impl_.get(), fx_handle);
            throw std::invalid_argument("Unknown EQ filter");
        }

        bqf.fBandwidth = NormalizeBqfBandwidth(fBandWidth);
        bqf.fS = fS;

        bqf.fCenter = fCenter;
        bqf.fGain = fGain;
        bqf.fQ = fQ;
        bqf.lChannel = BASS_BFX_CHANALL;

        XAMP_LOG_D(logger_, "{} fBandwidth:{}, fCenter:{}, fGain:{}, fQ:{} fS:{}",
            filter, bqf.fBandwidth, bqf.fCenter, bqf.fGain, bqf.fQ, bqf.fS);

        try {
            BassIfFailedThrow(LIB_BASS.BASS_FXSetParameters(fx_handle, &bqf));
            fx_handles_.push_back(fx_handle);
        } catch (...) {
            LIB_BASS.BASS_ChannelRemoveFX(impl_.get(), fx_handle);
            throw;
        }
    }

    void setEq(const EqSettings& settings) {
        // Build off-stream: failures leave the active filter chain untouched.
        BassParametricEqImpl candidate;
        candidate.start(sample_rate_, channels_);
        candidate.applyEq(settings);
        std::swap(impl_, candidate.impl_);
        std::swap(preamp_, candidate.preamp_);
        fx_handles_.swap(candidate.fx_handles_);
    }

    void applyEq(const EqSettings& settings) {
        for (const auto& band_setting : settings.bands) {
            AddBand(band_setting.type,
                band_setting.frequency,
                band_setting.band_width,
                band_setting.gain,
                band_setting.Q,
                band_setting.shelf_slope);
        }
        SetPreamp(settings.preamp);
    }

    void SetPreamp(float preamp) {
        if (!std::isfinite(preamp) || !std::isfinite(std::pow(10.0f, preamp / 20.0f)))
            throw std::invalid_argument("Invalid EQ preamp");
        BASS_BFX_VOLUME fv{};
        fv.lChannel = 0;
        fv.fVolume = static_cast<float>(std::pow(10, (preamp / 20)));
        BassIfFailedThrow(LIB_BASS.BASS_FXSetParameters(preamp_, &fv));
        XAMP_LOG_D(logger_, "Preamp {:.02} dB", preamp);
    }

    bool process(float const* samples, size_t num_samples, BufferRef<float>& out) {
        return bass_util::readStream(impl_, samples, num_samples, out);
    }

    uint32_t process(float const* samples, float* out, size_t num_samples) {
        return bass_util::readStream(impl_, samples, out, num_samples);
    }

private:
    void RemoveBandFx() {
        for (const auto fx_handle : fx_handles_) {
            LIB_BASS.BASS_ChannelRemoveFX(impl_.get(), fx_handle);
        }
        fx_handles_.clear();
    }

    void RemoveFx() {
        RemoveBandFx();
        LIB_BASS.BASS_ChannelRemoveFX(impl_.get(), preamp_);
        preamp_ = 0;
    }

    uint32_t sample_rate_{ 0 };
    uint32_t channels_{ 0 };
    BassStreamHandle impl_;
    HFX preamp_;
    std::vector<HFX> fx_handles_;
    LoggerPtr logger_;
};

BassParametricEq::BassParametricEq()
    : impl_(makeAlign<BassParametricEqImpl>()) {
}

XAMP_PIMPL_IMPL(BassParametricEq)

void BassParametricEq::initialize(const Property& config) {
    const auto output_format = config.get<AudioFormat>(DspConfig::kOutputFormat);
    impl_->start(output_format.getSampleRate(), output_format.getChannels());

    const auto settings = config.get<EqSettings>(DspConfig::kEQSettings);
    setEq(settings);    
}

void BassParametricEq::setEq(const EqSettings& settings) {
    impl_->setEq(settings);
}

bool BassParametricEq::process(float const* samples, size_t num_samples, BufferRef<float>& out)  {
    return impl_->process(samples, num_samples, out);
}

XAMP_STREAM_NAMESPACE_END
