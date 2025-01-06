#include <QPainter>
#include <QLinearGradient>
#include <QTimer>
#include <QPainterPath>
#include <QMouseEvent>

#include <widget/util/str_util.h>
#include <widget/chatgpt/waveformwidget.h>
#include <thememanager.h>

namespace {
    void GetChannelPeaks(const std::vector<float>& buffer, std::vector<float>& left_peeks, std::vector<float>& right_peaks) {
        if (buffer.size() < 2) {
            return;
        }
        if (left_peeks.empty()) {
            left_peeks.reserve(WaveformWidget::kFramesPerPeak);
        }
        if (right_peaks.empty()) {
            right_peaks.reserve(WaveformWidget::kFramesPerPeak);
        }
        const auto sign_mask = _mm256_castsi256_ps(_mm256_set1_epi32(0x7FFFFFFF));
        const int frame_count = buffer.size() / 2;

        for (int start = 0; start < frame_count; start += WaveformWidget::kFramesPerPeak) {
            float max_left = 0.f;
            float max_right = 0.f;
            int end = (std::min)(start + WaveformWidget::kFramesPerPeak, frame_count);

            // Use SIMD to find the maximum values
            __m256 max_left_vec = _mm256_setzero_ps();
            __m256 max_right_vec = _mm256_setzero_ps();

            for (int f = start; f < end; f += 8) {
                __m256 left_vec = _mm256_loadu_ps(&buffer[f * 2]);
                __m256 right_vec = _mm256_loadu_ps(&buffer[f * 2 + 8]);

                left_vec = _mm256_and_ps(left_vec, sign_mask);
                right_vec = _mm256_and_ps(right_vec, sign_mask);

                max_left_vec = _mm256_max_ps(max_left_vec, left_vec);
                max_right_vec = _mm256_max_ps(max_right_vec, right_vec);
            }

            // Reduce the SIMD vectors to find the maximum values
            alignas(16) float max_left_arr[8];
            alignas(16) float max_right_arr[8];
            _mm256_storeu_ps(max_left_arr, max_left_vec);
            _mm256_storeu_ps(max_right_arr, max_right_vec);

            max_left = *std::max_element(max_left_arr, max_left_arr + 8);
            max_right = *std::max_element(max_right_arr, max_right_arr + 8);

            left_peeks.push_back(max_left);
            right_peaks.push_back(max_right);
        }
    }
}

WaveformWidget::WaveformWidget(QWidget *parent)
    : QFrame(parent) {
    setStyleSheet("background-color: transparent; border: none;"_str);
}

void WaveformWidget::setCurrentPosition(float sec) {
	cursor_ms_ = sec * 1000.f;
	update();
}

void WaveformWidget::setSampleRate(uint32_t sample_rate) {
	sample_rate_ = sample_rate;
    left_peaks_.clear();
    right_peaks_.clear();
    updateWavePixmap();
	update();
}

void WaveformWidget::onReadAudioData(const std::vector<float> & buffer) {
	GetChannelPeaks(buffer, left_peaks_, right_peaks_);
    update();
}

void WaveformWidget::clear() {
	left_peaks_.clear();
	right_peaks_.clear();
    update();
}

void WaveformWidget::drawTimeAxis(QPainter& p) {
    auto total_sec = static_cast<int32_t>(total_ms_ / 1000.0f);
    if (total_sec <= 0) {
        return;
    }

    int h = height();
    int y_base = h - 1;

    p.setPen(QPen(Qt::lightGray, 1, Qt::SolidLine));
    p.drawLine(QPointF(0, y_base), QPointF(width(), y_base));

    auto f = qTheme.monoFont();
    f.setPointSize(8);
    p.setFont(f);

    int text_offset_y = -10;

    QFontMetrics fm(p.font());

    for (auto cur_sec = 0; cur_sec <= total_sec; ++cur_sec) {
        if (cur_sec == 0) {
            continue;
        }

        float x_tick = timeToX(cur_sec);

        if (cur_sec % 5 == 0) {
            p.drawLine(QPointF(x_tick, y_base), QPointF(x_tick, y_base - 2));
        }

        if (cur_sec % 30 == 0) {
            p.drawLine(QPointF(x_tick, y_base), QPointF(x_tick, y_base - 5));

            QString tick_text = formatDuration(cur_sec);
            int tw = fm.horizontalAdvance(tick_text);

            float x_text = x_tick - tw * 0.5f;

            if (x_text < 0) {
                x_text = 0;
            }
            else if (x_text + tw > width()) {
                x_text = width() - tw;
            }
            float y_text = y_base + text_offset_y;
            p.drawText(QPointF(x_text, y_text), tick_text);
        }
    }
}

void WaveformWidget::drawDuration(QPainter& painter) {
	if (cursor_ms_ < 0.f) {
        return;
	}

    float cur_sec = cursor_ms_ / 1000.f;
    float x_cursor = timeToX(cur_sec);

    painter.setPen(QPen(QColor(100, 200, 255), 1));
    painter.drawLine(QPointF(x_cursor, 0), QPointF(x_cursor, height()));

    QString time_text = formatDuration(cur_sec, false);

    QFontMetrics fm(painter.font());
    int text_width = fm.horizontalAdvance(time_text);
    int text_height = fm.height();

    float x_text = x_cursor - text_width - 15.0f;
    if (x_text < 0) {
        x_text = 0;
    }

    static const float y_text = 20.0f;
    static const int padding = 4;
    static const float corner_radius = 5.0f;

    QRectF bg_rect(x_text, y_text - text_height, text_width + padding * 2, text_height + padding * 2);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(50, 50, 200, 200));
    painter.drawRoundedRect(bg_rect, corner_radius, corner_radius);

    painter.setPen(Qt::white);
    float text_baseline_y = (y_text + padding - fm.descent());
    float text_start_x = x_text + padding;
    painter.drawText(QPointF(text_start_x, text_baseline_y), time_text);
}

void WaveformWidget::paintEvent(QPaintEvent *event) {
    QPainter painter(this);

    if (pixmap_dirty_ && is_read_completed_) {
        updateWavePixmap();
    }

    painter.drawPixmap(0, 0, wave_cache_);

    drawDuration(painter);
	drawTimeAxis(painter);
}

float WaveformWidget::xToTime(float x) const {
    float w = static_cast<float>(width());
    if (w <= 1.f) 
        return 0.f;
    const float ratio = x / w; // [0..1]
    float sec_range = (total_ms_ / 1000.f); // total_ms_ ->ms, /1000 => sec
    return ratio * sec_range;
}

float WaveformWidget::timeToX(float sec) const {
    if (peak_count_ < 2) 
        return 0.f;
    float w = static_cast<float>(width());
    float total_sec = total_ms_ / 1000.f; // total ms => s
    if (total_sec <= 0.f) 
        return 0.f;
    float ratio = sec / total_sec; // [0..1]
    return ratio * w;
}

void WaveformWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        float x = static_cast<float>(event->pos().x());
        float sec = xToTime(x);
		emit playAt(sec);
    }
}

void WaveformWidget::mouseMoveEvent(QMouseEvent* event) {
}

void WaveformWidget::mouseReleaseEvent(QMouseEvent* event) {
}

void WaveformWidget::resizeEvent(QResizeEvent* event) {
	QFrame::resizeEvent(event);
	if (total_ms_ == 0.0f) {
		return;
	}
	pixmap_dirty_ = true;
	update();
}

void WaveformWidget::updateWavePixmap() {
    wave_cache_ = QPixmap(size());
    wave_cache_.fill(Qt::black);

    if (left_peaks_.empty() || right_peaks_.empty()) {
        return;
    }

    QPainter p(&wave_cache_);
    p.setRenderHint(QPainter::Antialiasing, true);

    int w = wave_cache_.width();
    int h = wave_cache_.height();

    int peak_count = static_cast<int>(left_peaks_.size());
    if (peak_count < 1) {
        return;
    }

    auto map_x = [&](int i) {
        if (peak_count <= 1) return 0.0f;
        return static_cast<float>(i) / (peak_count - 1) * (w - 1);
        };

    // 上(左聲道)
    {
        QPolygonF poly;
        poly.reserve(peak_count * 2);
        int top = 0, channelH = h / 2;

        // 正半
        for (int i = 0; i < peak_count; i++) {
            float val = left_peaks_[i];
            float x = map_x(i);
            float y = mapPeakToY(val, top, channelH, true);
            poly.push_back(QPointF(x, y));
        }
        // 負半
        for (int i = peak_count - 1; i >= 0; i--) {
            float val = left_peaks_[i];
            float x = map_x(i);
            float y = mapPeakToY(val, top, channelH, false);
            poly.push_back(QPointF(x, y));
        }

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(128, 128, 128, 180));
        p.drawPolygon(poly);
    }

    // 下(右聲道)
    {
        QPolygonF poly;
        poly.reserve(peak_count * 2);
        int top = h / 2;
        int channelH = h / 2;

        for (int i = 0; i < peak_count; i++) {
            float val = right_peaks_[i];
            float x = map_x(i);
            float y = mapPeakToY(val, top, channelH, true);
            poly.push_back(QPointF(x, y));
        }
        for (int i = peak_count - 1; i >= 0; i--) {
            float val = right_peaks_[i];
            float x = map_x(i);
            float y = mapPeakToY(val, top, channelH, false);
            poly.push_back(QPointF(x, y));
        }

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 192, 192, 180));
        p.drawPolygon(poly);
    }

    // => 到此, wave 已畫好在 wave_cache_
    // 下次 paintEvent() 只需 drawPixmap 就行
    pixmap_dirty_ = false;
    peak_count_ = left_peaks_.size();
    total_ms_ = static_cast<float>(peak_count_) * (kFramesPerPeak * (1000.f / sample_rate_));
}

void WaveformWidget::doneRead() {
	is_read_completed_ = true;
    updateWavePixmap();
}

float WaveformWidget::mapPeakToY(float peakVal, int top, int height, bool isPositive) const {
    float headroomFactor = 0.6f;
    float midY = top + height * 0.5f;
    float amplitudeRange = (height * 0.5f) * headroomFactor;

    if (isPositive) {
        return midY - peakVal * amplitudeRange;
    }
    else {
        return midY + peakVal * amplitudeRange;
    }
}

