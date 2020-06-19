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
inline constexpr auto maxSize = 100;

Spectrograph::Spectrograph(QWidget* parent)
    : QWidget(parent)
    , low_freq_(0)
    , high_freq_(0)
    , frequency_(0)
    , max_lufs_(0)
    , dbm_(0)
    , lufs_count_(0) {
    lufs_series_ = new QSplineSeries();
    lufs_series_->setUseOpenGL(true);

    dbm_series_ = new QSplineSeries();
    dbm_series_->setUseOpenGL(true);

    chart_ = new QChart();
    chart_->setTheme(QtCharts::QChart::ChartThemeLight);
    chart_->addSeries(lufs_series_);
    chart_->addSeries(dbm_series_);
    chart_->legend()->hide();
    chart_->createDefaultAxes();
    chart_->axisX()->setRange(0, maxX);
    chart_->axisY()->setRange(-20, 5);

    chart_view_ = new QChartView(chart_);
    chart_view_->setRenderHint(QPainter::Antialiasing);

    auto layout = new QHBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(chart_view_);
    setLayout(layout);

    timer_.setTimerType(Qt::PreciseTimer);
    timer_.setInterval(300);

    reset_timer_.setTimerType(Qt::PreciseTimer);
    reset_timer_.setInterval(3000);

    (void)QObject::connect(&reset_timer_, &QTimer::timeout, [this]() {
        max_lufs_ = 0.0;
        lufs_count_ = 0;
    });

    (void)QObject::connect(&timer_, &QTimer::timeout, [this]() {
        chart_view_->setUpdatesEnabled(false);

        if (lufs_count_ == 0) {
            if (lufs_data_.size() == 0) {
                lufs_data_ << 0;
            }
            else {
                lufs_data_ << lufs_data_.back();
            }            
        }
        else {
            lufs_data_ << (max_lufs_ / lufs_count_);
        }

        dbm_data_ << dbm_;

        while (lufs_data_.size() > maxSize) {
            lufs_data_.removeFirst();
            dbm_data_.removeFirst();
        }

        lufs_series_->clear();
        dbm_series_->clear();

        const auto dx = maxX / (maxSize - 1);
        const auto less = maxSize - lufs_data_.size();

        for (auto i = 0; i < lufs_data_.size(); ++i) {            
            lufs_series_->append(less * dx + i * dx, lufs_data_.at(i));
            dbm_series_->append(less * dx + i * dx, dbm_data_.at(i));
        }

        chart_view_->setUpdatesEnabled(true);
    });

    (void)QObject::connect(&processor,
        &FFTProcessor::spectrumDataChanged,
        this,
        &Spectrograph::updateBar);

    reset_timer_.start();
    timer_.start();    
    thread_.start();
}

void Spectrograph::setFrequency(float low_freq, float high_freq, float frequency) {
    low_freq_ = low_freq;
    high_freq_ = high_freq;
    frequency_ = frequency;   
    processor.setFrequency(frequency);
}

void Spectrograph::updateBar(std::vector<SpectrumData> const& spectrum_data) {
    auto total_dbm = 0.0F;
    for (auto data : spectrum_data) {
        if (data.frequency >= low_freq_ && data.frequency < high_freq_) {
            max_lufs_ += data.lufs;
            total_dbm += data.dbm;
            ++lufs_count_;
        }
    }
    dbm_ = total_dbm / spectrum_data.size();
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
