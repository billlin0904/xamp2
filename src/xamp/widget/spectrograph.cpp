#include <QPainter>
#include <widget/spectrograph.h>

inline constexpr auto kMaxBar = 512;

Spectrograph::Spectrograph(QWidget* parent)
    : QWidget(parent) {
    setMinimumHeight(100);
}

void Spectrograph::init() {
    bars_.resize(kMaxBar);
    updateBars();
}

void Spectrograph::onDisplayChanged(std::vector<float> const& display) {
    display_ = display;
    updateBars();
}

void Spectrograph::updateBars() {
    bars_.fill(Bar());
    update();
}

void Spectrograph::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);

    auto num_bars = bars_.size();
    if (!num_bars) {
        return;
    }
	
    const auto widget_width = rect().width();
    const auto bar_plus_gap_width = widget_width / num_bars;
    const auto bar_width = 0.8 * bar_plus_gap_width;
    const auto gap_width = bar_plus_gap_width - bar_width;
    const auto padding_width = widget_width - num_bars * (bar_width + gap_width);
    const auto left_padding_width = (padding_width + gap_width) / 2;
    const auto bar_height = rect().height() - 2 * gap_width;

    QColor bar_color(51, 204, 102);
    QColor clip_color(255, 255, 0);
	
    for (auto i = 0; i < num_bars; ++i) {
        const qreal value = bars_[i].value;
        Q_ASSERT(value >= 0.0 && value <= 1.0);
        QRect bar = rect();
        bar.setLeft(rect().left() + left_padding_width + (i * (gap_width + bar_width)));
        bar.setWidth(bar_width);
        bar.setTop(rect().top() + gap_width + (1.0 - value) * bar_height);
        bar.setBottom(rect().bottom() - gap_width);

        auto color = bar_color;
        if (bars_[i].clipped)
            color = clip_color;

        painter.fillRect(bar, color);
    }
}