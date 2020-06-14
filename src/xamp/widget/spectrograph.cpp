#include <QPainter>
#include <QPaintEvent>
#include <base/math.h>

#include <widget/spectrograph.h>

inline constexpr size_t kBarCount = 120;
inline constexpr size_t kFFTSize = 2048;

Spectrograph::Spectrograph(QWidget* parent)
    : QFrame(parent) {
    fft_ = xamp::base::MakeAlign<xamp::player::FFT>(kFFTSize);
    bars_.resize(kBarCount);
    timer_.setTimerType(Qt::PreciseTimer);
    timer_.setInterval(33);
    (void)QObject::connect(&timer_, &QTimer::timeout, [this]() {
        for (auto &bar : bars_) {
            bar.value -= 1;
        }
        update();
    });
    timer_.start();
}

void Spectrograph::setFrequency(float low_freq, float high_freq, float frequency) {
    low_freq_ = low_freq;
    high_freq_ = high_freq;
    frequency_ = frequency;
    spectrum_data_.resize(kFFTSize);
}

void Spectrograph::paintEvent(QPaintEvent*) {
    QPainter painter(this);

    auto num_bars = static_cast<int32_t>(bars_.size());
    const auto widgetWidth = rect().width();
    const auto barPlusGapWidth = widgetWidth / num_bars;
    const auto barWidth = 0.8 * barPlusGapWidth;
    const auto gapWidth = barPlusGapWidth - barWidth;
    const auto paddingWidth = widgetWidth - num_bars * (barWidth + gapWidth);
    const auto leftPaddingWidth = (paddingWidth + gapWidth) / 2;
    const auto barHeight = rect().height() - 2 * gapWidth;

    int i = 0;
    for (const auto &value : bars_) {
        auto bar_rect = rect();
        bar_rect.setLeft(rect().left() + leftPaddingWidth + (i * (gapWidth + barWidth)));
        bar_rect.setWidth(barWidth);
        bar_rect.setTop(rect().top() + gapWidth + (1.0 - value.value) * barHeight);
        bar_rect.setBottom(rect().bottom() - gapWidth);
        painter.fillRect(bar_rect, Qt::gray);
        ++i;
    }

}

size_t Spectrograph::barIndex(float frequency) const {
    const auto bandWidth = float(high_freq_ - low_freq_) / bars_.size();
    const auto index = (frequency - low_freq_) / bandWidth;
    return static_cast<size_t>(index);
}

void Spectrograph::updateBar() {
    for (auto spectrum_data : spectrum_data_) {
        if (spectrum_data.frequency >= low_freq_ && spectrum_data.frequency < high_freq_) {
            auto &bar = bars_[barIndex(spectrum_data.frequency)];
            bar.fallout = 5;
            bar.value = (std::max)(bar.value, spectrum_data.magnitude);
            bar.clipped |= spectrum_data.clipped;
        }
    }

    update();
}

void Spectrograph::spectrumDataChanged(std::vector<float> const &samples) {
    /*
    static float total_time = 0.0;
    auto sin_wave = samples;
    float amplitude = 0.5;
    float frequency = 500;
    float phase = 0.0;

    if (total_time >= std::numeric_limits<float>::max()) {
        total_time = 0.0;
    }

    std::transform(samples.begin(), samples.end(), sin_wave.begin(), [=](auto ) {
        float value = amplitude * std::sin(2 * xamp::base::kPI * frequency * total_time + phase);
        total_time += (1.0F / 44100);
        return value;
    });

    auto result = fft_->Forward(sin_wave.data(), sin_wave.size());
    */

    auto result = fft_->Forward(samples.data(), samples.size());

    for (size_t i = 2; i <= result.size() / 2; ++i) {
        spectrum_data_[i].frequency = float(i * frequency_) / result.size();
        auto magnitude = std::hypot(result[i].imag(), result[i].real()) / 5;
        magnitude = (std::max)(0.0F, magnitude);
        magnitude = (std::min)(1.0F, magnitude);
        spectrum_data_[i].clipped = magnitude > 1.0F;
        spectrum_data_[i].magnitude = magnitude;
        spectrum_data_[i].phase = std::atan2(result[i].imag(), result[i].real());
    }

    updateBar();
}

void Spectrograph::reset() {
}

void Spectrograph::start() {
}

void Spectrograph::stop() {
}
