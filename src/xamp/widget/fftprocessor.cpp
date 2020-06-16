#include <widget/fftprocessor.h>

inline constexpr size_t kFFTSize = 2048;

static float Power2loudness(float power) {
    return 10 * std::log10(power) - 0.691;
}

static float Loudness2power(float loudness) {
    return pow(10, (loudness + 0.691) / 10.);
}

FFTProcessor::FFTProcessor(QObject* parent)
	: QObject(parent) 
    , frequency_(0) {
	fft_ = xamp::base::MakeAlign<xamp::player::FFT>(kFFTSize);
	spectrum_data_.resize(kFFTSize);
}

void FFTProcessor::setFrequency(float frequency) {
    frequency_ = frequency;
}

void FFTProcessor::OnSampleDataChanged(std::vector<float> const& samples) {
    auto result = fft_->Forward(samples.data(), samples.size());

    for (size_t i = 2; i <= result.size() / 2; ++i) {
        spectrum_data_[i].frequency = float(i * frequency_) / result.size();
        spectrum_data_[i].lufs = Power2loudness(result[i].imag() * result[i].imag() + result[i].real() * result[i].real());
        spectrum_data_[i].magnitude = std::hypot(result[i].imag(), result[i].real());
        spectrum_data_[i].phase = std::atan2(result[i].imag(), result[i].real());
    }

    emit spectrumDataChanged(spectrum_data_);
}
