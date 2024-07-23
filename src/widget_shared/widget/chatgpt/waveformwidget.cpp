#include <QPainter>
#include <QLinearGradient>

#include <widget/chatgpt/waveformwidget.h>

WaveformWidget::WaveformWidget(QWidget *parent)
    : QWidget(parent) {
}

WaveformWidget::~WaveformWidget() {
}

void WaveformWidget::readAudioData(const std::vector<int16_t> &buffer) {
    buffer_.insert(buffer_.end(), buffer.begin(), buffer.end());

    auto max_buffer_size = width();
    if (buffer_.size() > max_buffer_size) {
        buffer_.erase(buffer_.begin(), buffer_.begin() + (buffer_.size() - max_buffer_size));
    }

    update();
}

void WaveformWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);

    QPainter painter(this);
    
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

    int columnWidth = 3;
    int spaceBetweenColumns = 2;
    int radius = 2;

    for (auto i = 0; i < buffer_.size(); i += columnWidth + spaceBetweenColumns) {
        int16_t sample = buffer_[i];
        int y = (sample * height) / 65536 + (height / 2);

        int rectHeight = std::abs(y - height / 2);
        int rectTop = (y < height / 2) ? y : height / 2;

        QRectF rect(i, rectTop, columnWidth, rectHeight);
        painter.drawRoundedRect(rect, radius, radius);
    }
}
