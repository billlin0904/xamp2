#include <QPainter>
#include <QPaintEvent>
#include <base/math.h>

#include <widget/spectrograph.h>

inline constexpr size_t kBarCount = 120;

Spectrograph::Spectrograph(QWidget* parent)
    : QFrame(parent)
    , low_freq_(0)
    , high_freq_(0)
    , frequency_(0) {
    bars_.resize(kBarCount);
    timer_.setTimerType(Qt::PreciseTimer);
    timer_.setInterval(33);
    (void)QObject::connect(&timer_, &QTimer::timeout, [this]() {
        update();
        for (auto& bar : bars_) {
            if (bar.value - 0.1 > 0.001) {
                bar.value -= 0.1;
            }
            else {
                bar.value = 0.001;
            }
        }
    });
    (void)QObject::connect(&processor,
        &FFTProcessor::spectrumDataChanged,
        this,
        &Spectrograph::updateBar);
    timer_.start();    
    thread_.start();
}

Spectrograph::~Spectrograph() {
    thread_.quit();
    thread_.terminate();
}

void Spectrograph::setFrequency(float low_freq, float high_freq, float frequency) {
    low_freq_ = low_freq;
    high_freq_ = high_freq;
    frequency_ = frequency;   
    processor.setFrequency(frequency);
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

void Spectrograph::updateBar(std::vector<SpectrumData> const& spectrum_data) {
    for (auto spectrum_data : spectrum_data) {
        if (spectrum_data.frequency >= low_freq_ && spectrum_data.frequency < high_freq_) {
            spectrum_data.magnitude /= 5.0F;
            spectrum_data.magnitude = (std::max)(0.0F, spectrum_data.magnitude);
            spectrum_data.magnitude = (std::min)(1.0F, spectrum_data.magnitude);
            auto &bar = bars_[barIndex(spectrum_data.frequency)];
            bar.value = (std::max)(bar.value, spectrum_data.magnitude);
        }
    }    
}

void Spectrograph::reset() {
}

void Spectrograph::start() {
    timer_.start();
}

void Spectrograph::stop() {
    timer_.stop();    
}
