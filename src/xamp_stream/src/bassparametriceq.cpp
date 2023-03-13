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
    BassParametricEqImpl() {
        logger_ = LoggerManager::GetInstance().GetLogger(kBassParametricEqLoggerName);
    }

    void Start(uint32_t sample_rate, uint32_t num_bands) {
        RemoveFx();

        stream_.reset(BASS.BASS_StreamCreate(sample_rate,
                                             AudioFormat::kMaxChannel,
                                             BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE,
                                             STREAMPROC_DUMMY,
                                             nullptr));

        fx_handles_.resize(num_bands);

        for (auto& fx_handle : fx_handles_) {
            fx_handle = BASS.BASS_ChannelSetFX(stream_.get(), BASS_FX_DX8_PARAMEQ, 0);
            if (!fx_handle) {
                throw BassException(BASS.BASS_ErrorGetCode());
            }
        }
    }

    bool Process(float const* samples, uint32_t num_samples, BufferRef<float>& out) {
        return BassUtiltis::Process(stream_, samples, num_samples, out);
    }

    uint32_t Process(float const* samples, float* out, uint32_t num_samples) {
        return BassUtiltis::Process(stream_, samples, out, num_samples);
    }

    void SetEQ(uint32_t index, uint32_t center, uint32_t band_width, float gain) {
        BASS_DX8_PARAMEQ eq{};
        BassIfFailedThrow(BASS.BASS_FXGetParameters(fx_handles_[index], &eq));
        eq.fGain = gain;
        eq.fBandwidth = band_width;
        eq.fCenter = center;
        BassIfFailedThrow(BASS.BASS_FXSetParameters(fx_handles_[index], &eq));
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
    const auto num_bands = config.Get<uint32_t>(DspConfig::kParametricEqBand);
    impl_->Start(output_format.GetSampleRate(), num_bands);
}

void BassParametricEq::Init(const AnyMap& config) {
}

void BassParametricEq::SetEQ(uint32_t index, uint32_t center, uint32_t band_width, float gain) {
    impl_->SetEQ(index, center, band_width, gain);
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

