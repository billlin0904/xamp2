#include <QPainter>
#include <QPaintEvent>
#include <QHBoxLayout>
#include <QAreaSeries>

#include <cassert>
#include <limits>

#include <base/math.h>
#include <widget/str_utilts.h>
#include <widget/spectrograph.h>

inline constexpr auto kMaxX = 300;
inline constexpr auto kMaxSize = 50;

inline constexpr auto kMinY = -3;
inline constexpr auto kMaxY = 3;

Spectrograph::Spectrograph(QWidget* parent)
    : QWidget(parent)
    , low_freq_(0)
    , high_freq_(0)
    , frequency_(0)
    , max_lufs_(0)
    , lufs_count_(0)
    , minY_(kMinY)
    , maxY_(kMaxY) {    
    lufs_series_ = new QSplineSeries();
    lufs_series_->setUseOpenGL(true);
    lufs_series_->setName(Q_UTF8("LUFS"));
    
    chart_ = new QChart();
    chart_->addSeries(lufs_series_);    
    chart_->legend()->hide();
    chart_->createDefaultAxes();
    chart_->axisX()->setRange(0, kMaxX);
    chart_->axisY()->setRange(minY_, maxY_);
    auto f = font();
    chart_->setFont(f);

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

    (void)QObject::connect(lufs_series_, &QSplineSeries::pointAdded, [this](int index) {
        const auto y = lufs_series_->at(index).y();

        if (y < minY_ || y > maxY_) {
            if (y < minY_)
                minY_ = y;
            if (y > maxY_)
                maxY_ = y;
            chart_->axisY()->setRange(minY_ - 3, maxY_ + 3); 
        }
        });

    (void)QObject::connect(&reset_timer_, &QTimer::timeout, [this]() {       
        max_lufs_ = 0.0;
        lufs_count_ = 0;
        });

    (void)QObject::connect(&timer_, &QTimer::timeout, [this]() {
        chart_view_->setUpdatesEnabled(false);

        if (lufs_count_ > 0) {
            assert(!std::isnan(max_lufs_ / lufs_count_));
            lufs_data_ << (max_lufs_ / lufs_count_);
        }
        else {
            if (lufs_data_.isEmpty()) {
                lufs_data_ << xamp::base::kMinDb;
            }
            else {
                lufs_data_ << lufs_data_.back();
            }            
        }

        while (lufs_data_.size() > kMaxSize) {
            lufs_data_.removeFirst();            
        }
        lufs_series_->clear();

        const auto dx = kMaxX / (kMaxSize - 1);
        const auto less = kMaxSize - lufs_data_.size();

        for (auto i = 0; i < lufs_data_.size(); ++i) {            
            lufs_series_->append(less * dx + i * dx, lufs_data_.at(i));
        }

        chart_view_->setUpdatesEnabled(true);       
    });

    (void)QObject::connect(&processor,
        &FFTProcessor::spectrumDataChanged,
        this,
        &Spectrograph::updateBar);

    reset_timer_.start();
    timer_.start(); 
    processor.moveToThread(&thread_);
    thread_.start();
}

void Spectrograph::setBackgroundColor(QColor color) {
    chart_->setBackgroundBrush(QBrush(color));
}

void Spectrograph::setFrequency(float low_freq, float high_freq, float frequency) {
    low_freq_ = low_freq;
    high_freq_ = high_freq;
    frequency_ = frequency;
    processor.setFrequency(frequency);
}

void Spectrograph::updateBar(std::vector<SpectrumData> const& spectrum_data) {    
    for (size_t i = 2; i <= spectrum_data.size() / 2; ++i) {
        assert(!std::isnan(spectrum_data[i].lufs));
        max_lufs_ += spectrum_data[i].lufs;
        lufs_count_++;
    }
}

void Spectrograph::start() {
    timer_.start();
}

void Spectrograph::stopThread() {
    timer_.stop();
    thread_.quit();
    thread_.wait();
}

void Spectrograph::stop() {
    lufs_data_.clear();
    max_lufs_ = 0.0F;
    lufs_count_ = 0;
    minY_ = kMinY;
    maxY_ = kMaxY;
    lufs_series_->clear();
}
