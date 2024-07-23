#include <QPainter>
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
    painter.fillRect(rect(), Qt::black);

    auto height = this->height();
    auto width = this->width();

    painter.setPen(Qt::green);

    if (buffer_.empty()) {
        return;
    }

    for (auto i = 0; i < buffer_.size(); ++i) {
        int16_t sample = buffer_[i];
        int y = (sample * height) / 65536 + (height / 2);
        painter.drawLine(i, height / 2, i, y);
    }
}
