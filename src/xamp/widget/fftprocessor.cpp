#include <base/math.h>
#include <widget/fftprocessor.h>

inline constexpr size_t kFFTSize = 2048;

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

    auto N = result.size();

    for (size_t i = 2; i <= N / 2; ++i) {
        spectrum_data_[i].frequency = float(i * frequency_) / N;
        spectrum_data_[i].magnitude = xamp::base::GetMag(result[i]);
        spectrum_data_[i].lufs = xamp::base::Power2Loudness(xamp::base::GetPower(result[i]));
    }

    emit spectrumDataChanged(spectrum_data_);
}
