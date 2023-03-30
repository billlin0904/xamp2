#include <stream/bassparametriceq.h>

#include <base/logger.h>
#include <base/logger_impl.h>
#include <stream/basslib.h>
#include <stream/bass_utiltis.h>
#include <stream/eqsettings.h>
#include <stream/bassparametriceq.h>

namespace xamp::stream {

XAMP_DECLARE_LOG_NAME(BassParametricEq);

class BassParametricEq::BassParametricEqImpl {
public:
    static constexpr auto kMaxBand = 10;

    BassParametricEqImpl() {
        logger_ = LoggerManager::GetInstance().GetLogger(kBassParametricEqLoggerName);
    }

    void Start(uint32_t sample_rate) {
        RemoveFx();

        stream_.reset(BASS.BASS_StreamCreate(sample_rate,
            AudioFormat::kMaxChannel,
            BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE,
            STREAMPROC_DUMMY,
            nullptr));

        preamp_ = BASS.BASS_ChannelSetFX(stream_.get(), BASS_FX_BFX_VOLUME, 0);
    }

    void AddBand(EQFilterTypes filter, uint32_t center, uint32_t band_width, float gain, float Q, float S) {
        auto fx_handle = BASS.BASS_ChannelSetFX(stream_.get(), BASS_FX_BFX_BQF, 1);
        if (!fx_handle) {
            throw BassException(BASS.BASS_ErrorGetCode());
        }

        BASS_BFX_BQF bqf{};

        switch (filter) {
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
        default:;
        }

        bqf.fBandwidth = band_width;
        bqf.fCenter = center;
        bqf.fGain = gain;
        bqf.fQ = Q;
        bqf.fS = S;
        bqf.lChannel = BASS_BFX_CHANALL;

        BassIfFailedThrow(BASS.BASS_FXSetParameters(fx_handle, &bqf));
        fx_handles_.push_back(fx_handle);
    }

    void SetBand(EQFilterTypes filter, uint32_t band, uint32_t center, uint32_t band_width, float gain, float Q, float S) const {
        BASS_BFX_BQF bqf{};
        BassIfFailedThrow(BASS.BASS_FXGetParameters(fx_handles_[band], &bqf));

        switch (filter) {
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
        default:;
        }

        bqf.fBandwidth = band_width;
        bqf.fCenter = center;
        bqf.fGain = gain;
        bqf.fQ = Q;
        bqf.fS = S;
        bqf.lChannel = BASS_BFX_CHANALL;

        BassIfFailedThrow(BASS.BASS_FXSetParameters(fx_handles_[band], &bqf));

        XAMP_LOG_D(logger_, "Bandwidth:{}, center:{}, gain:{}, Q:{} S:{}",
            band_width, center, gain, Q, S);
    }

    void SetEq(EqSettings const& settings) {
        uint32_t i = 0;
        for (const auto& band_setting : settings.bands) {
            AddBand(band_setting.type, band_setting.frequency, band_setting.band_width, band_setting.gain, band_setting.Q, 0);
        }
        SetPreamp(settings.preamp);
    }

    void SetPreamp(float preamp) {
        BASS_BFX_VOLUME fv;
        fv.lChannel = 0;
        fv.fVolume = static_cast<float>(std::pow(10, (preamp / 20)));
        BassIfFailedThrow(BASS.BASS_FXSetParameters(preamp_, &fv));
    }

    bool Process(float const* samples, uint32_t num_samples, BufferRef<float>& out) {
        return BassUtiltis::Process(stream_, samples, num_samples, out);
    }

    uint32_t Process(float const* samples, float* out, uint32_t num_samples) {
        return BassUtiltis::Process(stream_, samples, out, num_samples);
    }

private:
    void RemoveFx() {
        for (auto fx_handle : fx_handles_) {
            BASS.BASS_ChannelRemoveFX(stream_.get(), fx_handle);
        }
        fx_handles_.clear();
        BASS.BASS_ChannelRemoveFX(stream_.get(), preamp_);
        preamp_ = 0;
    }

    BassStreamHandle stream_;
    HFX preamp_;
    std::vector<HFX> fx_handles_;
    LoggerPtr logger_;
};

BassParametricEq::BassParametricEq()
    : impl_(MakePimpl<BassParametricEqImpl>()) {
}

XAMP_PIMPL_IMPL(BassParametricEq)

void BassParametricEq::Start(const AnyMap& config) {
    const auto output_format = config.Get<AudioFormat>(DspConfig::kOutputFormat);
    impl_->Start(output_format.GetSampleRate());
}

void BassParametricEq::Init(const AnyMap& config) {
}

void BassParametricEq::SetBand(EQFilterTypes filter, uint32_t band, uint32_t center, uint32_t band_width, float gain, float Q, float S) {
    impl_->SetBand(filter, band, center, band_width, gain, Q, S);
}

bool BassParametricEq::Process(float const* samples, uint32_t num_samples, BufferRef<float>& out)  {
    return impl_->Process(samples, num_samples, out);
}

uint32_t BassParametricEq::Process(float const* samples, float* out, uint32_t num_samples) {
    return impl_->Process(samples, out, num_samples);
}

Uuid BassParametricEq::GetTypeId() const {
    return XAMP_UUID_OF(BassParametricEq);
}

std::string_view BassParametricEq::GetDescription() const noexcept {
    return "BassParametricEq";
}

void BassParametricEq::Flush() {
	
}


}

