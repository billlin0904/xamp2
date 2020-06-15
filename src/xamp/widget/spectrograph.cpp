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
        update();
        for (auto& bar : bars_) {
            if (bar.value - 0.1 > 0.0) {
                bar.value -= 0.1;
            }            
        }
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
    const auto widget_width = rect().width();
    const auto bar_plus_gap_width = widget_width / num_bars;
    const auto bar_width = 0.8 * bar_plus_gap_width;
    const auto gap_width = bar_plus_gap_width - bar_width;
    const auto padding_width = widget_width - num_bars * (bar_width + gap_width);
    const auto left_padding_width = (padding_width + gap_width) / 2;
    const auto bar_height = rect().height() - 2 * gap_width;

    int i = 0;
    for (const auto &value : bars_) {
        auto bar_rect = rect();
        bar_rect.setLeft(rect().left() + left_padding_width + (i++ * (gap_width + bar_width)));
        bar_rect.setWidth(bar_width);
        bar_rect.setTop(rect().top() + gap_width + (1.0 - value.value) * bar_height);
        bar_rect.setBottom(rect().bottom() - gap_width);
        painter.fillRect(bar_rect, Qt::gray);
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
            bar.value = (std::max)(bar.value, spectrum_data.magnitude);
        }
    }    
}

void Spectrograph::spectrumDataChanged(std::vector<float> const &samples) {
    auto result = fft_->Forward(samples.data(), samples.size());

    for (size_t i = 2; i <= result.size() / 2; ++i) {
        auto magnitude = std::hypot(result[i].imag(), result[i].real()) / 5;
        magnitude = (std::max)(0.0F, magnitude);
        magnitude = (std::min)(1.0F, magnitude);
        spectrum_data_[i].frequency = float(i * frequency_) / result.size();
        spectrum_data_[i].magnitude = magnitude;
        spectrum_data_[i].phase = std::atan2(result[i].imag(), result[i].real());
    }

    updateBar();
}

void Spectrograph::reset() {
}

void Spectrograph::start() {
    timer_.start();
}

void Spectrograph::stop() {
    timer_.stop();
}
