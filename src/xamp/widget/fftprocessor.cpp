#include <widget/fftprocessor.h>

inline constexpr size_t kFFTSize = 2048;

static float Power2Db(float power) {
    return 10 * std::log10(power);
}

static float Power2Dbm(float power) {
    return 10 * std::log10(power) + 30.0F;
}

static float Power2Loudness(float power) {
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
    absolute_threshold_ = Power2Loudness(-70.0F);
	spectrum_data_.resize(kFFTSize);
}

void FFTProcessor::setFrequency(float frequency) {
    frequency_ = frequency;
}

std::tuple<float, size_t> FFTProcessor::GetThreshold(float threshold) {
    float sum = 0;
    size_t N = 0;
    for (const auto& spectrum_data : spectrum_data_) {
        if (spectrum_data.power >= threshold) {
            sum += spectrum_data.power;
            ++N;
        }
    }
    return std::make_tuple(sum, N);
}

void FFTProcessor::OnSampleDataChanged(std::vector<float> const& samples) {
    auto result = fft_->Forward(samples.data(), samples.size());

    auto N = result.size();

    for (size_t i = 2; i <= N / 2; ++i) {
        spectrum_data_[i].frequency = float(i * frequency_) / N;

        const auto magnitude = GetMag(result[i]);
        spectrum_data_[i].magnitude = magnitude;
        spectrum_data_[i].db = GetDb(magnitude);
        spectrum_data_[i].power = Db2Power(spectrum_data_[i].db);
        spectrum_data_[i].amplitude = GetAmp(result[i], N);
        spectrum_data_[i].lufs = Power2Loudness(spectrum_data_[i].power);
        spectrum_data_[i].phase = GetPhase(result[i]);
    }

    auto [absolute_sum, absolute_n] = GetThreshold(absolute_threshold_);
    integrated_loudness_.push_back(Power2Loudness(absolute_n ? absolute_sum / absolute_n : absolute_threshold_));

    auto threshold = absolute_n ? (std::max)(absolute_sum / absolute_n / 100, absolute_threshold_) : absolute_threshold_;
    auto [threshold_sum, threshold_n] = GetThreshold(threshold);
    

    emit spectrumDataChanged(spectrum_data_);
}
