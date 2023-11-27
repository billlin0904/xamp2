#include <base/logger.h>
#include <base/logger_impl.h>
#include <stream/basslib.h>
#include <stream/bass_utiltis.h>
#include <stream/eqsettings.h>
#include <stream/bassparametriceq.h>

XAMP_STREAM_NAMESPACE_BEGIN

XAMP_DECLARE_LOG_NAME(BassParametricEq);

class BassParametricEq::BassParametricEqImpl {
public:
    BassParametricEqImpl()
		: preamp_(0) {
	    logger_ = LoggerManager::GetInstance().GetLogger(kBassParametricEqLoggerName);
    }

    void Start(uint32_t sample_rate) {
        RemoveFx();

        stream_.reset(BASS.BASS_StreamCreate(sample_rate,
            AudioFormat::kMaxChannel,
            BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE,
            STREAMPROC_DUMMY,
            nullptr));
        BASS_IF_FAILED_THROW(stream_);

        preamp_ = BASS.BASS_ChannelSetFX(stream_.get(), BASS_FX_BFX_VOLUME, 0);
        BASS_IF_FAILED_THROW(preamp_);

        sample_rate_ = sample_rate;
    }

    void AddBand(EQFilterTypes filter, float freq, float band_width, float gain, float Q, float S) {
	    const auto fx_handle = BASS.BASS_ChannelSetFX(stream_.get(), BASS_FX_BFX_BQF, 1);
        BASS_IF_FAILED_THROW(fx_handle);

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

        bqf.fBandwidth = 0;
        bqf.fS = 0;

        bqf.fCenter = freq;
        bqf.fGain = gain;
        bqf.fQ = Q;
        bqf.lChannel = BASS_BFX_CHANALL;

        XAMP_LOG_D(logger_, "{} Bandwidth:{}, freq:{}, gain:{}, Q:{} S:{}",
            filter, bqf.fBandwidth, bqf.fCenter, bqf.fGain, bqf.fQ, bqf.fS);

    	BASS_IF_FAILED_THROW(BASS.BASS_FXSetParameters(fx_handle, &bqf));

        fx_handles_.push_back(fx_handle);
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
        BASS_IF_FAILED_THROW(BASS.BASS_FXSetParameters(preamp_, &fv));
    }

    bool Process(float const* samples, uint32_t num_samples, BufferRef<float>& out) {
        return bass_utiltis::Process(stream_, samples, num_samples, out);
    }

    uint32_t Process(float const* samples, float* out, uint32_t num_samples) {
        return bass_utiltis::Process(stream_, samples, out, num_samples);
    }

private:
    void RemoveFx() {
        for (const auto fx_handle : fx_handles_) {
            BASS.BASS_ChannelRemoveFX(stream_.get(), fx_handle);
        }
        fx_handles_.clear();
        BASS.BASS_ChannelRemoveFX(stream_.get(), preamp_);
        preamp_ = 0;
    }

    uint32_t sample_rate_{ 0 };
    BassStreamHandle stream_;
    HFX preamp_;
    std::vector<HFX> fx_handles_;
    LoggerPtr logger_;
};

BassParametricEq::BassParametricEq()
    : impl_(MakeAlign<BassParametricEqImpl>()) {
}

XAMP_PIMPL_IMPL(BassParametricEq)

void BassParametricEq::Start(const AnyMap& config) {
    const auto output_format = config.Get<AudioFormat>(DspConfig::kOutputFormat);
    impl_->Start(output_format.GetSampleRate());
}

void BassParametricEq::Init(const AnyMap& config) {
    const auto settings = config.Get<EqSettings>(DspConfig::kEQSettings);
    SetEq(settings);
}

void BassParametricEq::SetEq(EqSettings const& settings) {
    impl_->SetEq(settings);
}

void BassParametricEq::SetBand(EQFilterTypes filter, uint32_t band, uint32_t center, uint32_t band_width, float gain, float Q, float S) {
    
}

bool BassParametricEq::Process(float const* samples, uint32_t num_samples, BufferRef<float>& out)  {
    return impl_->Process(samples, num_samples, out);
}

Uuid BassParametricEq::GetTypeId() const {
    return XAMP_UUID_OF(BassParametricEq);
}

std::string_view BassParametricEq::GetDescription() const noexcept {
    return "BassParametricEq";
}

XAMP_STREAM_NAMESPACE_END
