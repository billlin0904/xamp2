#include <QPainter>
#include <QPaintEvent>
#include <QHBoxLayout>
#include <QAreaSeries>

#include <base/math.h>
#include <widget/str_utilts.h>
#include <widget/spectrograph.h>

inline constexpr auto kBarCount = 120;
inline constexpr auto maxX = 300;
inline constexpr auto maxY = 100;
inline constexpr auto maxSize = 50;

Spectrograph::Spectrograph(QWidget* parent)
    : QWidget(parent)
    , low_freq_(0)
    , high_freq_(0)
    , frequency_(0)
    , max_lufs_(-1000.0F) {
    bars_.resize(kBarCount);
    spline_series_ = new QSplineSeries();

    chart_ = new QChart();
    chart_->setTheme(QtCharts::QChart::ChartThemeBlueNcs);
    chart_->addSeries(spline_series_);
    chart_->legend()->hide();
    chart_->createDefaultAxes();
    chart_->axisX()->setRange(0, maxX);
    chart_->axisY()->setRange(0, maxY);

    chart_view_ = new QChartView(chart_);
    chart_view_->setRenderHint(QPainter::Antialiasing);

    auto layout = new QHBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(chart_view_);
    setLayout(layout);

    timer_.setTimerType(Qt::PreciseTimer);
    timer_.setInterval(300);
    (void)QObject::connect(&timer_, &QTimer::timeout, [this]() {
        data_ << max_lufs_;

        while (data_.size() > maxSize) {
            data_.removeFirst();
        }

        spline_series_->clear();

        for (auto i = 0; i < data_.size(); ++i) {
            size_t dx = maxX / (maxSize - 1);
            size_t less = maxSize - data_.size();
            spline_series_->append(less * dx + i * dx, data_.at(i));
        }

        max_lufs_ = 0.0;
    });
    (void)QObject::connect(&processor,
        &FFTProcessor::spectrumDataChanged,
        this,
        &Spectrograph::updateBar);
    timer_.start();    
    thread_.start();
}

void Spectrograph::setFrequency(float low_freq, float high_freq, float frequency) {
    low_freq_ = low_freq;
    high_freq_ = high_freq;
    frequency_ = frequency;   
    processor.setFrequency(frequency);
}

size_t Spectrograph::barIndex(float frequency) const {
    const auto bandWidth = float(high_freq_ - low_freq_) / bars_.size();
    const auto index = (frequency - low_freq_) / bandWidth;
    return static_cast<size_t>(index);
}

void Spectrograph::updateBar(std::vector<SpectrumData> const& spectrum_data) {
    for (auto data : spectrum_data) {
        if (data.frequency >= low_freq_ && data.frequency < high_freq_) {
            data.magnitude /= 5.0F;
            data.magnitude = (std::max)(0.0F, data.magnitude);
            data.magnitude = (std::min)(1.0F, data.magnitude);
            auto &bar = bars_[barIndex(data.frequency)];
            bar.value = (std::max)(bar.value, data.magnitude);
            max_lufs_ = (std::max)(max_lufs_, data.lufs);
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
    thread_.quit();
    thread_.wait();
}
