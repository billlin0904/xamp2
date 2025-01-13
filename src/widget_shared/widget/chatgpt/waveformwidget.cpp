#include <QPainter>
#include <QLinearGradient>
#include <QTimer>
#include <QMouseEvent>

#include <widget/util/str_util.h>
#include <widget/chatgpt/waveformwidget.h>
#include <widget/actionmap.h>
#include <widget/appsettings.h>
#include <widget/appsettingnames.h>
#include <thememanager.h>

namespace {
    void GetChannelPeaks(const std::vector<float>& buffer, size_t frame_per_peek, std::vector<float>& left_peeks, std::vector<float>& right_peaks) {
        if (buffer.size() < 2) {
            return;
        }

    	if (left_peeks.empty()) {
            left_peeks.reserve(frame_per_peek * 8);
        }
        if (right_peaks.empty()) {
            right_peaks.reserve(frame_per_peek * 8);
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

    float mapX(int i, int peak_count, int widget_width) {
        if (peak_count <= 1) return 0.f;
        return static_cast<float>(i) / (peak_count - 1) * (widget_width - 1);
    }
}

WaveformWidget::WaveformWidget(QWidget *parent)
    : QFrame(parent) {
    setStyleSheet("background-color: transparent; border: none;"_str);
    setContextMenuPolicy(Qt::CustomContextMenu);
    draw_mode_ = DRAW_SPECTROGRAM;//qAppSettings.valueAsInt(kAppSettingWaveformDrawMode);

    (void)QObject::connect(this, &WaveformWidget::customContextMenuRequested, [this](auto pt) {
        ActionMap<WaveformWidget> action_map(this);

		auto setting_value = qAppSettings.valueAsInt(kAppSettingWaveformDrawMode);
        auto show_ch_menu = action_map.addSubMenu(tr("Show Channel"));

        auto* only_right_ch_act = show_ch_menu->addAction(tr("Show Only Right Channel"), [this]() {
            auto enable = draw_mode_ & DRAW_PLAYED_AREA;
            setDrawMode(DRAW_ONLY_RIGHT_CHANNEL | (enable ? DRAW_PLAYED_AREA : 0));
            });
		if (setting_value & DRAW_ONLY_RIGHT_CHANNEL) {
            only_right_ch_act->setChecked(true);
		}
        auto* only_left_ch_act = show_ch_menu->addAction(tr("Show Only Left Channel"), [this]() {
            auto enable = draw_mode_& DRAW_PLAYED_AREA;
            setDrawMode(DRAW_ONLY_LEFT_CHANNEL | (enable ? DRAW_PLAYED_AREA : 0));
            });
        if (setting_value & DRAW_ONLY_LEFT_CHANNEL) {
            only_left_ch_act->setChecked(true);
        }
        auto* only_both_ch_act = show_ch_menu->addAction(tr("Show Both Channel"), [this]() {
            auto enable = draw_mode_ & DRAW_PLAYED_AREA;
            setDrawMode(DRAW_BOTH_CHANNEL | (enable ? DRAW_PLAYED_AREA : 0));
            });
        if (setting_value & DRAW_BOTH_CHANNEL) {
            only_both_ch_act->setChecked(true);
        }
        auto submenu = action_map.addSubMenu(tr("Show Played Area"));
        auto* show_act = submenu->addAction(tr("Enable"), [this]() {
			uint32_t mode = 0;
			if (draw_mode_ & DRAW_PLAYED_AREA) {
                mode = draw_mode_ & ~DRAW_PLAYED_AREA;
			}
			else {
                mode = draw_mode_ | DRAW_PLAYED_AREA;
			}
			setDrawMode(mode);
            });
        if (setting_value & DRAW_PLAYED_AREA) {
            show_act->setChecked(true);
        }
        action_map.exec(pt);
        update();
        });
}

void WaveformWidget::setCurrentPosition(float sec) {
	cursor_ms_ = sec * 1000.f;

    if (left_peaks_.empty() || right_peaks_.empty()) {
        update();
        return;
    }

    QRectF old_left_box = path_left_played_.boundingRect();
    QRectF old_right_box = path_right_played_.boundingRect();

    if (draw_mode_ & DRAW_PLAYED_AREA) {
        float ratio = cursor_ms_ / total_ms_;
        ratio = std::clamp(ratio, 0.f, 1.f);
        const int play_index = static_cast<int>(std::floor(ratio * peak_count_));
        updatePlayedPaths(play_index);
    }
    
	update();
}

void WaveformWidget::setTotalDuration(float duration) {
	total_ms_ = duration * 1000.f;
    peak_count_ = 2;
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

void WaveformWidget::setDrawMode(uint32_t mode) {
    draw_mode_ = mode;
    updateCachePixmap();
    qAppSettings.setValue(kAppSettingWaveformDrawMode, mode);
	update();
}

void WaveformWidget::setSpectrogramData(const QImage& spectrogramImg) {
    spectrogram_ = QPixmap::fromImage(spectrogramImg);
    updateSpectrogramSize();
    update();
}

void WaveformWidget::onReadAudioData(const std::vector<float> & buffer) {
	GetChannelPeaks(buffer, frame_per_peak_, left_peaks_, right_peaks_);
    update();
}

void WaveformWidget::updateCachePixmap() {
    if (left_peaks_.empty() || right_peaks_.empty()) {
        return;
    }

    const int w = width();
    const int h = height();

    QImage img(w, h, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);

    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHints(QPainter::SmoothPixmapTransform);

    const int pc = static_cast<int>(left_peaks_.size());
    if (pc < 1) return;

    // 幫助函式: 將 [0..pc] 建出路徑(整條)
    auto draw_one_channel = [&](const std::vector<float>& peaks,
        const QColor& color,
        int top,
        int channelH)
        {
            // 正半
            QPainterPath path;
            float x0 = mapX(0, pc, w);
            float y0 = mapPeakToY(peaks[0], top, channelH, true);
            path.moveTo(x0, y0);

            for (int i = 1; i < pc; i++) {
                float x = mapX(i, pc, w);
                float y = mapPeakToY(peaks[i], top, channelH, true);
                path.lineTo(x, y);
            }
            // 反半
            for (int i = pc - 1; i >= 0; i--) {
                float x = mapX(i, pc, w);
                float y = mapPeakToY(peaks[i], top, channelH, false);
                path.lineTo(x, y);
            }
            path.closeSubpath();

            p.setPen(Qt::NoPen);
            p.setBrush(color);
            p.drawPath(path);
        };

    // 依 draw_mode_ 不同, 我們決定 top / channelH
    if (draw_mode_ & DRAW_BOTH_CHANNEL) {
        // 左聲道 (上半)
        draw_one_channel(left_peaks_,
            kLeftUnPlayedChannelColor,  // 灰
            0,    // top=0
            h / 2); // channelH=一半

        // 右聲道 (下半)
        draw_one_channel(right_peaks_,
            kRightUnPlayedChannelColor,    // 青
            h / 2, // top=中間開始
            h / 2);
    }
    else if (draw_mode_ & DRAW_ONLY_LEFT_CHANNEL) {
        // 只繪製 left_peaks_, 但「佔滿整個height()」
        draw_one_channel(left_peaks_,
            kLeftUnPlayedChannelColor,
            0,  // top=0
            h); // 全高
    }
    else if (draw_mode_ & DRAW_ONLY_RIGHT_CHANNEL) {
        // 只繪製 right_peaks_, 佔滿整個height()
        draw_one_channel(right_peaks_,
            kRightUnPlayedChannelColor,
            0,
            h);
    }

	p.end();
    cache_ = QPixmap::fromImage(img);

    // 計算 total_ms_
    peak_count_ = pc;
    total_ms_ = static_cast<float>(peak_count_)
		* (frame_per_peak_ * (1000.f / static_cast<float>(sample_rate_)));
}

QPainterPath WaveformWidget::buildChannelPath(const std::vector<float>& peaks, int startIndex, int endIndex, int top, int channelH) const {
    QPainterPath path;
    if (endIndex <= startIndex) return path;

    const int total_count = static_cast<int>(peaks.size());
    if (startIndex < 0) startIndex = 0;
    if (endIndex > total_count) endIndex = total_count;
    if (endIndex <= startIndex) return path;

    auto mapIndexToX = [&](int i) {
        if (total_count <= 1) return 0.f;
        return static_cast<float>(i) / (total_count - 1) * (width() - 1);
        };

    // 正半
    float x0 = mapIndexToX(startIndex);
    float y0 = mapPeakToY(peaks[startIndex], top, channelH, true);
    path.moveTo(x0, y0);

    for (int i = startIndex + 1; i < endIndex; i++) {
        float x = mapIndexToX(i);
        float y = mapPeakToY(peaks[i], top, channelH, true);
        path.lineTo(x, y);
    }

    // 反半
    for (int i = endIndex - 1; i >= startIndex; i--) {
        float x = mapIndexToX(i);
        float y = mapPeakToY(peaks[i], top, channelH, false);
        path.lineTo(x, y);
    }
    path.closeSubpath();
    return path;
}

void WaveformWidget::updatePlayedPaths(int playIndex) {
    // 如果 playIndex<=0 => 沒有已播放
    if (playIndex <= 0) {
        path_left_played_ = QPainterPath();
        path_right_played_ = QPainterPath();
        return;
    }
    // clamp
    if (playIndex > static_cast<int>(left_peaks_.size())) {
        playIndex = static_cast<int>(left_peaks_.size());
    }

    const int h = height();

    // 若 DRAW_BOTH_CHANNEL => 左聲道是 top=0, channelH=h/2
    //        右聲道是 top=h/2, channelH=h/2
    // 若 ONLY_LEFT => top=0, channelH=h
    // 若 ONLY_RIGHT => top=0, channelH=h

    if (draw_mode_ & DRAW_BOTH_CHANNEL) {
        path_left_played_ = buildChannelPath(left_peaks_, 0, playIndex,
            /*top=*/0,    /*channelH=*/h / 2);
        path_right_played_ = buildChannelPath(right_peaks_, 0, playIndex,
            /*top=*/h / 2,  /*channelH=*/h / 2);
    }
    else if (draw_mode_ & DRAW_ONLY_LEFT_CHANNEL) {
        path_left_played_ = buildChannelPath(left_peaks_, 0, playIndex,
            0, h);
        path_right_played_ = QPainterPath();
    }
    else if (draw_mode_ & DRAW_ONLY_RIGHT_CHANNEL) {
        path_left_played_ = QPainterPath();
        path_right_played_ = buildChannelPath(right_peaks_, 0, playIndex,
            0, h);
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

    painter.setRenderHints(QPainter::Antialiasing 
        | QPainter::SmoothPixmapTransform 
        | QPainter::TextAntialiasing);

    painter.setClipRegion(event->region());

    if (draw_mode_ & DRAW_SPECTROGRAM) {
        painter.drawPixmap(0, 0, spectrogram_cache_);
    } else {
        painter.drawPixmap(0, 0, cache_);
    }

	if (draw_mode_ & DRAW_PLAYED_AREA) {
        if (draw_mode_ & DRAW_BOTH_CHANNEL || draw_mode_ & DRAW_ONLY_LEFT_CHANNEL) {
			painter.setPen(Qt::NoPen);
			painter.setBrush(kLeftPlayedChannelColor);
			painter.drawPath(path_left_played_);
		}
		if (draw_mode_ & DRAW_BOTH_CHANNEL || draw_mode_ & DRAW_ONLY_RIGHT_CHANNEL) {
			painter.setPen(Qt::NoPen);
			painter.setBrush(kRightPlayedChannelColor);
			painter.drawPath(path_right_played_);
		}
	}   

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
    if (draw_mode_ & DRAW_SPECTROGRAM) {
		updateSpectrogramSize();
	} else {
        updateCachePixmap();
	}
	update();
}

void WaveformWidget::doneRead() {
    if (draw_mode_ & DRAW_SPECTROGRAM) {
        updateSpectrogramSize();
    }
    else {
        updateCachePixmap();
    }
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

void WaveformWidget::updateSpectrogramSize() {
    if (spectrogram_.isNull())
        return;
    spectrogram_cache_ = spectrogram_.scaled(size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
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
