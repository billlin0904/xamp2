#include <base/logger.h>
#include <base/logger_impl.h>
#include <stream/basslib.h>
#include <stream/bass_utiltis.h>
#include <stream/eqsettings.h>
#include <stream/bassequalizer.h>

XAMP_STREAM_NAMESPACE_BEGIN

XAMP_DECLARE_LOG_NAME(BassEqualizer);

class BassEqualizer::BassEqualizerImpl {
public:
    BassEqualizerImpl()
		: preamp_(0) {
        logger_ = XampLoggerFactory.GetLogger(kBassEqualizerLoggerName);
    }

    void Start(uint32_t sample_rate) {
        RemoveFx();

        stream_.reset(BASS.BASS_StreamCreate(sample_rate,
                                             AudioFormat::kMaxChannel,
                                             BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE,
                                             STREAMPROC_DUMMY,
                                             nullptr));
        BassIfFailedThrow(stream_);

        preamp_ = BASS.BASS_ChannelSetFX(stream_.get(), BASS_FX_BFX_VOLUME, 0);
        BassIfFailedThrow(preamp_);
    }

    void AddBand(uint32_t i, float freq, float band_width, float gain, float Q) {
        const auto fx_handle = BASS.BASS_ChannelSetFX(stream_.get(), BASS_FX_BFX_PEAKEQ, 1);
        BassIfFailedThrow(fx_handle);

        BASS_BFX_PEAKEQ eq{};
        eq.lBand = i;
        eq.fCenter = freq;
        eq.fBandwidth = (freq / Q);
        eq.fGain = gain;
        eq.fQ = Q;
        eq.lChannel = BASS_BFX_CHANALL;
        BassIfFailedThrow(BASS.BASS_FXSetParameters(fx_handle, &eq));
        fx_handles_.push_back(fx_handle);
        XAMP_LOG_D(logger_, "Add band {}Hz {}dB Q:{} Bandwidth:{} successfully!", freq, gain, Q, eq.fBandwidth);
    }

    bool Process(float const* samples, uint32_t num_samples, BufferRef<float>& out) {
        return bass_utiltis::Process(stream_, samples, num_samples, out);
    }

    void SetEq(EqSettings const &settings) {
        SetPreamp(settings.preamp);
        uint32_t i = 0;
        for (const auto &band_setting : settings.bands) {
            AddBand(i++, band_setting.frequency, band_setting.band_width, band_setting.gain, band_setting.Q);
        }
    }

    void SetEq(uint32_t band, float gain, float Q) {
        if (band >= fx_handles_.size()) {
            return;
        }

        BASS_BFX_PEAKEQ eq{};
        BassIfFailedThrow(BASS.BASS_FXGetParameters(fx_handles_[band], &eq));
        eq.fGain = gain;
        eq.fQ = Q;
        BassIfFailedThrow(BASS.BASS_FXSetParameters(fx_handles_[band], &eq));
    }

    void SetPreamp(float preamp_db) {
        BASS_BFX_VOLUME fv{};
        fv.lChannel = BASS_BFX_CHANALL;
        fv.fVolume = static_cast<float>(std::pow(10, (preamp_db / 20)));
        BassIfFailedThrow(BASS.BASS_FXSetParameters(preamp_, &fv));
        XAMP_LOG_D(logger_, "Add preamp {}dB successfully!", preamp_db);
    }

    void Disable() {
        for (uint32_t i = 0; i < fx_handles_.size(); ++i) {
            SetEq(i, 0.0, 0.0);
        }
        SetPreamp(0);
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

    HFX preamp_;
    BassStreamHandle stream_;
    Vector<HFX> fx_handles_;
    LoggerPtr logger_;
};

BassEqualizer::BassEqualizer()
    : impl_(MakeAlign<BassEqualizerImpl>()) {
}

XAMP_PIMPL_IMPL(BassEqualizer)

void BassEqualizer::Start(const AnyMap& config) {
    const auto output_format = config.Get<AudioFormat>(DspConfig::kOutputFormat);
    impl_->Start(output_format.GetSampleRate());
}

void BassEqualizer::Init(const AnyMap& config) {
	const auto settings = config.Get<EqSettings>(DspConfig::kEQSettings);
    SetEq(settings);
}

void BassEqualizer::SetEq(uint32_t band, float gain, float Q) {
    impl_->SetEq(band, gain, Q);
}

void BassEqualizer::SetEq(EqSettings const & settings) {
    impl_->SetEq(settings);
}

void BassEqualizer::SetPreamp(float preamp) {
    impl_->SetPreamp(preamp);
}

bool BassEqualizer::Process(float const* samples, uint32_t num_samples, BufferRef<float>& out)  {
    return impl_->Process(samples, num_samples, out);
}

Uuid BassEqualizer::GetTypeId() const {
    return XAMP_UUID_OF(BassEqualizer);
}

std::string_view BassEqualizer::GetDescription() const noexcept {
    return Description;
}

XAMP_STREAM_NAMESPACE_END

