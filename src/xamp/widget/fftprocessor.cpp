#include <widget/fftprocessor.h>

inline constexpr size_t kFFTSize = 2048;

static float Power2Db(float power) {
    return 10 * std::log10(power);
}

static float Power2Dbm(float power) {
    return 10 * std::log10(power) + 30.0F;
}

static float Power2Loudness(float power) {
    if (power == 0.0F) {
        return -100.0F;
    }
    return 10 * std::log10(power) - 0.691F;
}

static float Loudness2Power(float loudness) {
    return std::pow(10, (loudness + 0.691F) / 10.0F);
}

static float GetMag(std::complex<float> &c) {
    return std::hypot(c.imag(), c.real());
}

static float GetAmp(std::complex<float>& c, size_t N) {
    return std::sqrt(
        std::pow(c.imag(), 2) + std::pow(c.real(), 2)
    ) / N;
}

static float GetDb(float magnitude) {
    return 20 * std::log10(magnitude);
}

static float Db2Power(float db) {
    return std::pow(10, db / 10.0F);
}

static float GetPhase(std::complex<float>& c) {
    return std::atan2(c.imag(), c.real());
}

FFTProcessor::FFTProcessor(QObject* parent)
	: QObject(parent) 
    , frequency_(0) {
	fft_ = xamp::base::MakeAlign<xamp::player::FFT>(kFFTSize);
    absolute_threshold_ = Loudness2Power(-70.0F);
    spectrum_data_.resize(kFFTSize);
    integrated_loudness_.reserve(kFFTSize);
    loudness_range_.reserve(kFFTSize);
}

void FFTProcessor::setFrequency(float frequency) {
    frequency_ = frequency;
}

std::tuple<float, size_t> FFTProcessor::GetThreshold(float threshold) {
    float sum = 0;
    size_t N = 0;
    for (const auto& spectrum_data : spectrum_data_) {
        if (spectrum_data.magnitude >= threshold) {
            sum += spectrum_data.magnitude;
            ++N;
        }
    }
    return std::make_tuple(sum, N);
}

void FFTProcessor::OnSampleDataChanged(std::vector<float> const& samples) {
    auto result = fft_->Forward(samples.data(), samples.size());

    if (integrated_loudness_.size() > kFFTSize) {
        integrated_loudness_.clear();
        loudness_range_.clear();
    }

    auto N = result.size();

    for (size_t i = 2; i <= N / 2; ++i) {
        spectrum_data_[i].frequency = float(i * frequency_) / N;

        const auto magnitude = GetMag(result[i]);
        spectrum_data_[i].magnitude = magnitude;
        spectrum_data_[i].lufs = Power2Loudness(
            static_cast<float>(std::pow(result[i].imag(), 2) + std::pow(result[i].real(), 2)));
    }

    emit spectrumDataChanged(spectrum_data_);
}
