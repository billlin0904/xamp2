#include <QPainter>
#include <QLinearGradient>
#include <QTimer>
#include <QPainterPath>

#include <widget/chatgpt/waveformwidget.h>

WaveformWidget::WaveformWidget(QWidget *parent)
    : QWidget(parent)
    , timer_(new QTimer(this)) {
    (void)QObject::connect(timer_, &QTimer::timeout, [this]() {
        update();
        });
    //timer_->start(30);
}

WaveformWidget::~WaveformWidget() {
}

void WaveformWidget::readAudioData(const std::vector<int16_t> &buffer) {
    buffer_.insert(buffer_.end(), buffer.begin(), buffer.end());

    auto max_buffer_size = width();
    //auto max_buffer_size = 360;
    if (buffer_.size() > max_buffer_size) {
        buffer_.erase(buffer_.begin(), buffer_.begin() + (buffer_.size() - max_buffer_size));
    }
    update();
}

void WaveformWidget::silence() {
    buffer_.erase(buffer_.begin(), buffer_.end());
    update();
}

void WaveformWidget::paintEvent(QPaintEvent *event) {
    QPainter painter(this);

    painter.setRenderHints(QPainter::Antialiasing, true);
    painter.setRenderHints(QPainter::SmoothPixmapTransform, true);
    painter.setRenderHints(QPainter::TextAntialiasing, true);

    //painter.fillRect(this->rect(), Qt::black);
    
    auto height = this->height();
    auto width = this->width();

    if (buffer_.empty()) {
        return;
    }

    QLinearGradient gradient(0, 0, width, 0);
    gradient.setColorAt(0.0, QColor(0, 120, 255));
    gradient.setColorAt(0.5, QColor(180, 50, 220));
    gradient.setColorAt(1.0, QColor(255, 50, 50));
    painter.setBrush(gradient);
    painter.setPen(Qt::NoPen);

    static constexpr auto kColumnWidth = 3;
    static constexpr auto kSpaceBetweenColumns = 2;
    static constexpr auto kRadius = 2;

    auto start_index = (std::max)(0, static_cast<int>(buffer_.size()) - this->width());

    for (auto i = 0; i < this->width() && (start_index + i) < buffer_.size(); i += kColumnWidth + kSpaceBetweenColumns) {
        auto sample = buffer_[start_index + i] * 10;

        auto y = (sample * height) / 65536 + (height / 2);
        
        auto rect_height = std::abs(y - height / 2);
        auto rect_top = (y < height / 2) ? y : height / 2;

        const QRectF rect(i, rect_top, kColumnWidth, rect_height);
        painter.drawRoundedRect(rect, kRadius, kRadius);
    }
}
