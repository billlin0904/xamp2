#include <base/logger.h>
#include <base/logger_impl.h>

#include <supereq.h>

#include <stream/basslib.h>
#include <stream/bass_utiltis.h>
#include <stream/eqsettings.h>
#include <stream/supereqequalizer.h>

XAMP_STREAM_NAMESPACE_BEGIN

XAMP_DECLARE_LOG_NAME(SuperEqEqualizer);

class SuperEqEqualizer::SuperEqEqualizerImpl {
public:
    SuperEqEqualizerImpl() {
        logger_ = LoggerManager::GetInstance().GetLogger(kSuperEqEqualizerLoggerName);
    }

    void Start(uint32_t sample_rate) {
        sample_rate_ = sample_rate;
        eqs_.clear();
        for (auto i = 0; i < AudioFormat::kMaxChannel; ++i) {
            eqs_.push_back(MakeAlign<supereq<float>>());
        }
        const auto settings = GetDefaultEqSettings();
        SetEq(settings);
    }

    bool Process(float const* samples, uint32_t num_samples, BufferRef<float>& out) {
	    const int32_t channel_samples = num_samples * AudioFormat::kMaxChannel;
        buffer_.resize(channel_samples);

        for (auto i = 0; i < AudioFormat::kMaxChannel; ++i) {
            for (auto s = 0; s < channel_samples; s++) {
                buffer_[s] = samples[s * AudioFormat::kMaxChannel + i];
            }
            eqs_[i]->write_samples(buffer_.data(), channel_samples);
        }

        int32_t read_samples = 0;

        for (auto i = 0; i < AudioFormat::kMaxChannel; ++i) {
            int32_t samples_out = 0;
            const auto *output = eqs_[i]->get_output(&samples_out);
            const auto excepted_size = (samples_out * AudioFormat::kMaxChannel) + read_samples;
            if (excepted_size > buffer_.size()) {
                buffer_.resize(excepted_size);
            }

            for (auto s = 0; s < samples_out; s++) {
                buffer_[s * AudioFormat::kMaxChannel + i] = output[s];
            }
            read_samples += samples_out;
        }

        if (read_samples > 0) {
            out.maybe_resize(read_samples);

            for (auto i = 0; i < read_samples; ++i) {
                out.data()[i] = buffer_[i];
            }
            return true;
        }

        return false;
    }

    void SetEq(const EqSettings & settings) {
        /*std::array<double, kMaxSuperEqBand> bands;
    	SetupBand(settings, bands);

        for (auto i = 0; i < AudioFormat::kMaxChannel; ++i) {
            eqs_[i]->equ_makeTable(bands.data(), &paramroot_, sample_rate_);
        }*/
    }

    void Flush() {
        for (auto i = 0; i < AudioFormat::kMaxChannel; ++i) {
            eqs_[i]->equ_clearbuf();
        }
    }

private:
    void SetupBand(const EqSettings& settings, std::array<double, kMaxSuperEqBand> &bands) {
        for (auto i = 0; i < settings.bands.size(); ++i) {
            bands[i] = std::pow(10.0, (settings.bands[i].gain - 20) / -20.0);
        }
    }
    uint32_t sample_rate_{ 0 };
    paramlist paramroot_;
    std::vector<float> buffer_;
    Vector<AlignPtr<supereq<float>>> eqs_;
    LoggerPtr logger_;
};

SuperEqEqualizer::SuperEqEqualizer()
    : impl_(MakeAlign<SuperEqEqualizerImpl>()) {
}

XAMP_PIMPL_IMPL(SuperEqEqualizer)

void SuperEqEqualizer::Start(const AnyMap& config) {
    const auto output_format = config.Get<AudioFormat>(DspConfig::kOutputFormat);
    impl_->Start(output_format.GetSampleRate());
}

void SuperEqEqualizer::Init(const AnyMap& config) {
    const auto settings = config.Get<EqSettings>(DspConfig::kEQSettings);
    impl_->SetEq(settings);
}

bool SuperEqEqualizer::Process(float const* samples, uint32_t num_samples, BufferRef<float>& out) {
    return impl_->Process(samples, num_samples, out);
}

Uuid SuperEqEqualizer::GetTypeId() const {
    return XAMP_UUID_OF(SuperEqEqualizer);
}

std::string_view SuperEqEqualizer::GetDescription() const noexcept {
    return "SuperEq";
}

void SuperEqEqualizer::Flush() {
}

EqSettings SuperEqEqualizer::GetDefaultEqSettings() {
    constexpr std::array<float, 18> band_freq{
    	55,
    	77,
    	110,
    	156,
    	220,
    	311,
    	440,
    	622,
    	880,
    	1200,
    	1800,
    	2500,
    	3500,
    	5000,
    	7000,
    	10000,
    	14000,
    	20000
    };

    constexpr std::array<float, 18> band_gain{
        2,1,0,0,0,-2,0,-2,0,0,0,0,2,0,0,3,0,0
    };

    auto i = 0;
    EqSettings settings;
    for (const auto freq : band_freq) {
        EqBandSetting band;
        band.frequency = freq;
        band.frequency = freq / 2;
        band.gain = band_gain[i++];
        band.Q = kDefaultQ;
        settings.bands.push_back(band);
    }
    return settings;
}

XAMP_STREAM_NAMESPACE_END

