#include <QPainter>
#include <QLinearGradient>
#include <QTimer>
#include <QPainterPath>
#include <QMouseEvent>

#include <widget/util/str_util.h>
#include <widget/chatgpt/waveformwidget.h>
#include <thememanager.h>

namespace {
    void GetChannelPeaks(const std::vector<float>& buffer, size_t frame_per_peek, std::vector<float>& left_peeks, std::vector<float>& right_peaks) {
        if (buffer.size() < 2) {
            return;
        }
        if (left_peeks.empty()) {
            left_peeks.reserve(frame_per_peek);
        }
        if (right_peaks.empty()) {
            right_peaks.reserve(frame_per_peek);
        }
        const auto sign_mask = _mm256_castsi256_ps(_mm256_set1_epi32(0x7FFFFFFF));
        const size_t frame_count = buffer.size() / 2;

        for (int start = 0; start < frame_count; start += frame_per_peek) {
            float max_left = 0.f;
            float max_right = 0.f;
            int end = (std::min)(start + frame_per_peek, frame_count);

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
    if (left_peaks_.empty() || right_peaks_.empty()) {
        return;
    }

	cursor_ms_ = sec * 1000.f;
    //float ratio = cursor_ms_ / total_ms_;
    //ratio = std::clamp(ratio, 0.f, 1.f);
	//const int play_index = static_cast<int>(std::floor(ratio * peak_count_));
    //updatePlayedPaths(play_index);
	update();
}

void WaveformWidget::setSampleRate(uint32_t sample_rate) {
	sample_rate_ = sample_rate;
    left_peaks_.clear();
    right_peaks_.clear();
	path_left_played_ = QPainterPath();
    path_right_played_ = QPainterPath();
	update();
}

void WaveformWidget::setFramePerPeekSize(size_t size) {
	frame_per_peak_ = size;
}

void WaveformWidget::onReadAudioData(const std::vector<float> & buffer) {
	GetChannelPeaks(buffer, frame_per_peak_, left_peaks_, right_peaks_);
    update();
}

void WaveformWidget::updateUnplayedPixmap() {
    cache_ = QPixmap(size());
    cache_.fill(Qt::black);

    if (left_peaks_.empty() || right_peaks_.empty()) {
        return;
    }

    QPainter p(&cache_);
    p.setRenderHint(QPainter::Antialiasing, true);

    int w = cache_.width();
    int h = cache_.height();

    int pc = static_cast<int>(left_peaks_.size());
    if (pc < 1) return;

    // 幫助：index->X
    auto map_x = [&](int i) {
        if (pc <= 1) return 0.f;
        return static_cast<float>(i) / (pc - 1) * (w - 1);
        };

    // 左聲道
    {
        // build a path from [0.. pc], (整條)
        QPainterPath path;

        // step A) 正半
        float x0 = map_x(0);
        float y0 = mapPeakToY(left_peaks_[0], 0, h / 2, true);
        path.moveTo(x0, y0);
        for (int i = 1; i < pc; i++) {
            float x = map_x(i);
            float y = mapPeakToY(left_peaks_[i], 0, h / 2, true);
            path.lineTo(x, y);
        }
        // step B) 反半
        for (int i = pc - 1; i >= 0; i--) {
            float x = map_x(i);
            float y = mapPeakToY(left_peaks_[i], 0, h / 2, false);
            path.lineTo(x, y);
        }
        path.closeSubpath();

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(128, 128, 128, 180)); // 灰
        p.drawPath(path);
    }

    // 右聲道
    {
        QPainterPath path;

        float x0 = map_x(0);
        float y0 = mapPeakToY(right_peaks_[0], h / 2, h / 2, true);
        path.moveTo(x0, y0);

        for (int i = 1; i < pc; i++) {
            float x = map_x(i);
            float y = mapPeakToY(right_peaks_[i], h / 2, h / 2, true);
            path.lineTo(x, y);
        }

        for (int i = pc - 1; i >= 0; i--) {
            float x = map_x(i);
            float y = mapPeakToY(right_peaks_[i], h / 2, h / 2, false);
            path.lineTo(x, y);
        }
        path.closeSubpath();

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 192, 192, 180)); // 青
        p.drawPath(path);
    }

	peak_count_ = left_peaks_.size();
    total_ms_ = static_cast<float>(peak_count_) * (frame_per_peak_ * (1000.f / static_cast<float>(sample_rate_)));
}

QPainterPath WaveformWidget::buildChannelPath(const std::vector<float>& peaks, int startIndex, int endIndex, int top, int channelH) const {
    QPainterPath path;
    if (endIndex <= startIndex) return path;

    const int total_count = static_cast<int>(peaks.size());
    if (startIndex < 0) startIndex = 0;
    if (endIndex > total_count) endIndex = total_count;
    if (endIndex <= startIndex) return path;

    auto map_x = [&](int i) {
        if (total_count <= 1) return 0.f;
        return static_cast<float>(i) / (total_count - 1) * (width() - 1);
        };

    // 正半
    float x0 = map_x(startIndex);
    float y0 = mapPeakToY(peaks[startIndex], top, channelH, true);
    path.moveTo(x0, y0);

    for (int i = startIndex + 1; i < endIndex; i++) {
        float x = map_x(i);
        float y = mapPeakToY(peaks[i], top, channelH, true);
        path.lineTo(x, y);
    }

    // 反半
    for (int i = endIndex - 1; i >= startIndex; i--) {
        float x = map_x(i);
        float y = mapPeakToY(peaks[i], top, channelH, false);
        path.lineTo(x, y);
    }
    path.closeSubpath();
    return path;
}

void WaveformWidget::updatePlayedPaths(int playIndex) {
    // left 
    if (playIndex > 0) {
        path_left_played_ = buildChannelPath(left_peaks_, 0, playIndex, 0, height() / 2);
    }
    else {
        path_left_played_ = QPainterPath();
    }

    // right
    if (playIndex > 0) {
        path_right_played_ = buildChannelPath(right_peaks_, 0, playIndex, height() / 2, height() / 2);
    }
    else {
        path_right_played_ = QPainterPath();
    }
}

void WaveformWidget::drawTimeAxis(QPainter& p) {
    auto total_sec = static_cast<int32_t>(total_ms_ / 1000.0f);
    if (total_sec <= 0) {
        return;
    }

    const bool min_range = total_sec <= 30;

    int h = height();
    int y_base = h - 1;

    p.setPen(QPen(Qt::lightGray, 1, Qt::SolidLine));
    p.drawLine(QPointF(0, y_base), QPointF(width(), y_base));

    auto f = qTheme.monoFont();
    f.setPointSize(8);
    p.setFont(f);

    const QFontMetrics fm(p.font());

    for (auto cur_sec = 0; cur_sec <= total_sec; ++cur_sec) {
        if (cur_sec == 0) {
            continue;
        }

        float x_tick = timeToX(cur_sec);

        if (cur_sec % 5 == 0) {
            p.drawLine(QPointF(x_tick, y_base), QPointF(x_tick, y_base - 2));
        }

        if (cur_sec % 30 == 0 || (min_range && cur_sec % 1 == 0)) {
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
            float y_text = y_base + kTextOffsetY;
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

	const QFontMetrics fm(painter.font());
    int text_width = fm.horizontalAdvance(time_text);
    int text_height = fm.height();

    float x_text = x_cursor - text_width - 15.0f;
    if (x_text < 0) {
        x_text = 0;
    }

    QRectF bg_rect(x_text, kYTextHeight - text_height, text_width + kPadding * 2, text_height + kPadding * 2);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(50, 50, 200, 200));
    painter.drawRoundedRect(bg_rect, kCornerRadius, kCornerRadius);

    painter.setPen(Qt::white);
    float text_baseline_y = (kYTextHeight + kPadding - fm.descent());
    float text_start_x = x_text + kPadding;
    painter.drawText(QPointF(text_start_x, text_baseline_y), time_text);
}

void WaveformWidget::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing, true);
    painter.setRenderHints(QPainter::SmoothPixmapTransform, true);
    painter.setRenderHints(QPainter::TextAntialiasing, true);

    painter.drawPixmap(0, 0, cache_);

    //painter.setPen(Qt::NoPen);
    //painter.setBrush(kLeftPlayedChannelColor);
    //painter.drawPath(path_left_played_);
    //painter.setBrush(kRightPlayedChannelColor);
    //painter.drawPath(path_right_played_);

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
    updateUnplayedPixmap();
	update();
}

void WaveformWidget::doneRead() {
    updateUnplayedPixmap();
    update();
}

void WaveformWidget::clear() {
    cache_ = QPixmap(size());
    cache_.fill(Qt::black);
    total_ms_ = 0.0f;
    cursor_ms_ = -1.f;
    peak_count_ = 0;
    left_peaks_.clear();
    right_peaks_.clear();
	path_left_played_ = QPainterPath();
    path_right_played_ = QPainterPath();
    update();
}

float WaveformWidget::mapPeakToY(float peakVal, int top, int height, bool isPositive) const {
    float mid_y = top + height * 0.5f;
    float amplitude_range = (height * 0.5f) * kHeadroomFactor;

    if (isPositive) {
        return mid_y - peakVal * amplitude_range;
    }
    else {
        return mid_y + peakVal * amplitude_range;
    }
}