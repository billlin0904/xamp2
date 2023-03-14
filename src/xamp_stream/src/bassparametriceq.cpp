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

        fx_handles_.resize(kMaxBand);

        for (auto& fx_handle : fx_handles_) {
            fx_handle = BASS.BASS_ChannelSetFX(stream_.get(), BASS_FX_BFX_BQF, 0);
            if (!fx_handle) {
                throw BassException(BASS.BASS_ErrorGetCode());
            }
        }
    }

    void SetBand(FilterTypes filter, uint32_t band, uint32_t center, uint32_t band_width, float gain, float Q, float S) const {
        BASS_BFX_BQF bqf{};
        BassIfFailedThrow(BASS.BASS_FXGetParameters(fx_handles_[band], &bqf));

        switch (filter) {
        case FilterTypes::FT_LOW_SHELF:
            bqf.lFilter = BASS_BFX_BQF_LOWSHELF;
            break;
        case FilterTypes::FT_LOW_HIGH_SHELF:
            bqf.lFilter = BASS_BFX_BQF_HIGHSHELF;
            break;
        case FilterTypes::FT_LOW_PASS:
            bqf.lFilter = BASS_BFX_BQF_LOWPASS;
            break;
        case FilterTypes::FT_HIGH_PASS:
            bqf.lFilter = BASS_BFX_BQF_HIGHPASS;
            break;
        case FilterTypes::FT_HIGH_BAND_PASS:
            bqf.lFilter = BASS_BFX_BQF_BANDPASS;
            break;
        case FilterTypes::FT_HIGH_BAND_PASS_Q:
            bqf.lFilter = BASS_BFX_BQF_BANDPASS_Q;
            break;
        case FilterTypes::FT_NOTCH:
            bqf.lFilter = BASS_BFX_BQF_NOTCH;
            break;
        case FilterTypes::FT_ALL_PASS:
            bqf.lFilter = BASS_BFX_BQF_ALLPASS;
            break;
        case FilterTypes::FT_ALL_PEAKING_EQ:
            bqf.lFilter = BASS_BFX_BQF_PEAKINGEQ;
            break;
        default:;
        }

        bqf.fBandwidth = band_width;
        bqf.fCenter = center;
        bqf.fGain = gain;
        bqf.fQ = Q;
        bqf.fS = S;

        BassIfFailedThrow(BASS.BASS_FXSetParameters(fx_handles_[band], &bqf));

        XAMP_LOG_D(logger_, "Bandwidth:{}, center:{}, gain:{}, Q:{} S:{}",
            band_width, center, gain, Q, S);
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
    }

    BassStreamHandle stream_;
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

void BassParametricEq::SetBand(FilterTypes filter, uint32_t band, uint32_t center, uint32_t band_width, float gain, float Q, float S) {
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

