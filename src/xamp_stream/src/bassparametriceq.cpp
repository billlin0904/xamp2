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
	    logger_ = XampLoggerFactory.GetLogger(XAMP_LOG_NAME(BassParametricEq));
    }
    
    void Start(uint32_t sample_rate) {
        RemoveFx();

        stream_.reset(BASS_LIB.BASS_StreamCreate(sample_rate,
            AudioFormat::kMaxChannel,
            BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE,
            STREAMPROC_DUMMY,
            nullptr));
        BassIfFailedThrow(stream_);

        preamp_ = BASS_LIB.BASS_ChannelSetFX(stream_.get(), BASS_FX_BFX_VOLUME, 0);
        BassIfFailedThrow(preamp_);

        sample_rate_ = sample_rate;
    }

    void AddBand(EQFilterTypes filter, float fCenter, float fBandWidth, float fGain, float fQ, float fS) {
	    const auto fx_handle = BASS_LIB.BASS_ChannelSetFX(stream_.get(), BASS_FX_BFX_BQF, 1);
        BassIfFailedThrow(fx_handle);

        BASS_BFX_BQF bqf{};

        switch (filter) {
        case EQFilterTypes::FT_LOW_SHELF:
            bqf.lFilter = BASS_BFX_BQF_LOWSHELF;
            fS = fQ;
            fQ = 0;
            break;
        case EQFilterTypes::FT_LOW_HIGH_SHELF:
            bqf.lFilter = BASS_BFX_BQF_HIGHSHELF;
            fS = fQ;
            fQ = 0;
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

        bqf.fBandwidth = fBandWidth;
        bqf.fS = fS;

        bqf.fCenter = fCenter;
        bqf.fGain = fGain;
        bqf.fQ = fQ;
        bqf.lChannel = BASS_BFX_CHANALL;

        XAMP_LOG_D(logger_, "{} fBandwidth:{}, fCenter:{}, fGain:{}, fQ:{} fS:{}",
            filter, bqf.fBandwidth, bqf.fCenter, bqf.fGain, bqf.fQ, bqf.fS);

    	BassIfFailedThrow(BASS_LIB.BASS_FXSetParameters(fx_handle, &bqf));

        fx_handles_.push_back(fx_handle);
    }

    void SetEq(const EqSettings& settings) {
        uint32_t i = 0;
        for (const auto& band_setting : settings.bands) {
            AddBand(band_setting.type, band_setting.frequency, band_setting.band_width, band_setting.gain, band_setting.Q, band_setting.shelf_slope);
        }
        SetPreamp(settings.preamp);
    }

    void SetPreamp(float preamp) {
        BASS_BFX_VOLUME fv;
        fv.lChannel = 0;
        fv.fVolume = static_cast<float>(std::pow(10, (preamp / 20)));
        BassIfFailedThrow(BASS_LIB.BASS_FXSetParameters(preamp_, &fv));
    }

    bool Process(float const* samples, size_t num_samples, BufferRef<float>& out) {
        return bass_utiltis::Process(stream_, samples, num_samples, out);
    }

    uint32_t Process(float const* samples, float* out, size_t num_samples) {
        return bass_utiltis::Process(stream_, samples, out, num_samples);
    }

private:
    void RemoveFx() {
        for (const auto fx_handle : fx_handles_) {
            BASS_LIB.BASS_ChannelRemoveFX(stream_.get(), fx_handle);
        }
        fx_handles_.clear();
        BASS_LIB.BASS_ChannelRemoveFX(stream_.get(), preamp_);
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

void BassParametricEq::Initialize(const AnyMap& config) {
    const auto output_format = config.Get<AudioFormat>(DspConfig::kOutputFormat);
    impl_->Start(output_format.GetSampleRate());

    const auto settings = config.Get<EqSettings>(DspConfig::kEQSettings);
    impl_->SetPreamp(settings.preamp);
    SetEq(settings);    
}

void BassParametricEq::SetEq(const EqSettings& settings) {
    impl_->SetEq(settings);
}

bool BassParametricEq::Process(float const* samples, size_t num_samples, BufferRef<float>& out)  {
    return impl_->Process(samples, num_samples, out);
}

Uuid BassParametricEq::GetTypeId() const {
    return XAMP_UUID_OF(BassParametricEq);
}

std::string_view BassParametricEq::GetDescription() const noexcept {
    return "BassParametricEq";
}

XAMP_STREAM_NAMESPACE_END
