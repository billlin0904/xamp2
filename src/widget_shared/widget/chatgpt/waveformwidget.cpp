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
    draw_mode_ = qAppSettings.valueAsInt(kAppSettingWaveformDrawMode);

    (void)QObject::connect(this, &WaveformWidget::customContextMenuRequested, [this](auto pt) {
        ActionMap<WaveformWidget> action_map(this);

		auto setting_value = qAppSettings.valueAsInt(kAppSettingWaveformDrawMode);
        auto show_ch_menu = action_map.addSubMenu(tr("Show Channel"));

		auto* show_spectrogram_act = show_ch_menu->addAction(tr("Show Spectrogram"), [this]() {
			setDrawMode(DRAW_SPECTROGRAM);
			});
        if (setting_value & DRAW_SPECTROGRAM) {
            show_spectrogram_act->setChecked(true);
        }

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

QRect WaveformWidget::drawRect() const {
    constexpr int leftMargin = 50;
    constexpr int rightMargin = 10;
    constexpr int topMargin = 10;
    constexpr int bottomMargin = 30;

    QRect waveform_rect(leftMargin,
        topMargin,
        width() - leftMargin - rightMargin,
        height() - topMargin - bottomMargin);

    return waveform_rect;
}

void WaveformWidget::updateCachePixmap() {
    // 如果沒有任何 peak，直接退出
    if (left_peaks_.empty() || right_peaks_.empty()) {
        return;
    }

    // === 1) 先決定 waveformRect，代表「白色框線」內部的繪製區域 ===
    QRect waveformRect = drawRect();
    // 若空間太小，不做繪製
    if (waveformRect.width() <= 0 || waveformRect.height() <= 0) {
        return;
    }

    // === 2) 建立與 waveformRect 等大的 QImage，用來畫波形的「背景快取」 ===
    QImage img(waveformRect.size(), QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);

    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    // peak_count 與計算後的總時長
    const int pc = static_cast<int>(left_peaks_.size());
    if (pc < 1) return;

    // === 3) 幫助函式：將某一聲道的 peaks 轉成「局部座標(0..img.width())」的路徑並繪製 ===
    auto draw_one_channel = [&](const std::vector<float>& peaks,
        const QColor& color,
        int topInRect,
        int channelHeight)
        {
            QPainterPath path;

            // 當 pc=peaks.size()，每個 i 映射到 [0 .. waveformRect.width()-1 ]
            // 為了簡化，最右可以用 (waveformRect.width()) 或 (waveformRect.width()-1) 都行
            // 下列 mapIndexToLocalX 即求「在 cache_ 圖片裡的 x」
            auto mapIndexToLocalX = [&](int i) {
                if (pc <= 1) return 0.f;
                return static_cast<float>(i) / (pc - 1)
                    * static_cast<float>(waveformRect.width() - 1);
                };

            // 先移動到 i=0 的位置(正半)
            float x0 = mapIndexToLocalX(0);
            float y0 = mapPeakToY(peaks[0], topInRect, channelHeight, true);
            path.moveTo(x0, y0);

            // 往右掃，畫「正半」
            for (int i = 1; i < pc; i++) {
                float x = mapIndexToLocalX(i);
                float y = mapPeakToY(peaks[i], topInRect, channelHeight, true);
                path.lineTo(x, y);
            }
            // 再往左掃，畫「負半」
            for (int i = pc - 1; i >= 0; i--) {
                float x = mapIndexToLocalX(i);
                float y = mapPeakToY(peaks[i], topInRect, channelHeight, false);
                path.lineTo(x, y);
            }
            path.closeSubpath();

            p.setPen(Qt::NoPen);
            p.setBrush(color);
            p.drawPath(path);
        };

    // === 4) 根據 draw_mode_，決定要畫哪幾個聲道 ===
    int h = waveformRect.height();
    if (draw_mode_ & DRAW_BOTH_CHANNEL) {
        // 左聲道 (上半)
        draw_one_channel(left_peaks_, kLeftUnPlayedChannelColor, 0, h / 2);
        // 右聲道 (下半)
        draw_one_channel(right_peaks_, kRightUnPlayedChannelColor, h / 2, h / 2);
    }
    else if (draw_mode_ & DRAW_ONLY_LEFT_CHANNEL) {
        draw_one_channel(left_peaks_, kLeftUnPlayedChannelColor, 0, h);
    }
    else if (draw_mode_ & DRAW_ONLY_RIGHT_CHANNEL) {
        draw_one_channel(right_peaks_, kRightUnPlayedChannelColor, 0, h);
    }

    p.end(); // 結束對 img 的繪製

    // === 5) 把繪製完成的波形快取存到 cache_ 裡，
    //     注意：cache_ 的尺寸僅是 waveformRect 的大小
    cache_ = QPixmap::fromImage(img);

    // === 6) 更新 peak_count_ 與 total_ms_ ===
    peak_count_ = pc;
    total_ms_ = static_cast<float>(peak_count_)
        * (frame_per_peak_ * (1000.f / static_cast<float>(sample_rate_)));
}

void WaveformWidget::updatePlayedPaths(int playIndex) {
    // 如果播放位置 <=0，表示沒播放
    if (playIndex <= 0) {
        path_left_played_ = QPainterPath();
        path_right_played_ = QPainterPath();
        return;
    }

    // clamp
    if (playIndex > static_cast<int>(left_peaks_.size())) {
        playIndex = static_cast<int>(left_peaks_.size());
    }

    // === 1) 先決定 waveformRect（同 paintEvent & updateCachePixmap）===
    QRect waveformRect = drawRect();

    // 如果太小，直接清空路徑
    if (waveformRect.width() <= 0 || waveformRect.height() <= 0) {
        path_left_played_ = QPainterPath();
        path_right_played_ = QPainterPath();
        return;
    }

    // === 2) 幫助函式：建立「已播放」區域的路徑 (絕對座標)
    auto buildChannelPath = [&](const std::vector<float>& peaks,
        int startIndex, int endIndex,
        int topInRect, int channelHeight)
        {
            QPainterPath path;
            if (endIndex <= startIndex) return path;

            int total_count = static_cast<int>(peaks.size());
            if (startIndex < 0) startIndex = 0;
            if (endIndex > total_count) endIndex = total_count;
            if (endIndex <= startIndex) return path;

            // 將索引 i → 「waveformRect 內的 x (絕對座標)」
            auto mapIndexToAbsX = [&](int i) {
                if (total_count <= 1) return static_cast<float>(waveformRect.left());
                float ratio = static_cast<float>(i) / (total_count - 1);
                float xRange = static_cast<float>(waveformRect.width() - 1);
                return waveformRect.left() + ratio * xRange;
                };

            // 正半
            float x0 = mapIndexToAbsX(startIndex);
            float y0 = waveformRect.top() + mapPeakToY(peaks[startIndex], topInRect, channelHeight, true);
            path.moveTo(x0, y0);

            for (int i = startIndex + 1; i < endIndex; ++i) {
                float x = mapIndexToAbsX(i);
                float y = waveformRect.top() + mapPeakToY(peaks[i], topInRect, channelHeight, true);
                path.lineTo(x, y);
            }

            // 負半
            for (int i = endIndex - 1; i >= startIndex; --i) {
                float x = mapIndexToAbsX(i);
                float y = waveformRect.top() + mapPeakToY(peaks[i], topInRect, channelHeight, false);
                path.lineTo(x, y);
            }
            path.closeSubpath();
            return path;
        };

    // === 3) 根據「顯示模式」決定要建哪個聲道的路徑 ===
    int h = waveformRect.height();
    if (draw_mode_ & DRAW_BOTH_CHANNEL) {
        // 左聲道 (上半)
        path_left_played_ = buildChannelPath(left_peaks_, 0, playIndex, 0, h / 2);
        // 右聲道 (下半)
        path_right_played_ = buildChannelPath(right_peaks_, 0, playIndex, h / 2, h / 2);
    }
    else if (draw_mode_ & DRAW_ONLY_LEFT_CHANNEL) {
        path_left_played_ = buildChannelPath(left_peaks_, 0, playIndex, 0, h);
        path_right_played_ = QPainterPath();
    }
    else if (draw_mode_ & DRAW_ONLY_RIGHT_CHANNEL) {
        path_left_played_ = QPainterPath();
        path_right_played_ = buildChannelPath(right_peaks_, 0, playIndex, 0, h);
    }
}

void WaveformWidget::drawTimeAxis(QPainter& painter, const QRect& rect) {
    // 取得總秒數
    int32_t total_sec = static_cast<int32_t>(total_ms_ / 1000.0f);
    if (total_sec <= 0) return;

    int axisY = rect.bottom();
    painter.setPen(QPen(Qt::white, 1));

    QFont f = painter.font();
    f.setPointSize(8);
    painter.setFont(f);
    QFontMetrics fm(painter.font());

    bool min_range = (total_sec <= 30);
    auto min_tick = 5;
	auto max_tick = 30;
    if (total_sec > 3600) {
		min_tick = 60;
		max_tick = 300;
    }

    for (int cur_sec = 0; cur_sec <= total_sec; ++cur_sec) {
        float ratio = static_cast<float>(cur_sec) / total_sec;
        float x_tick = rect.left() + ratio * rect.width();

        if (cur_sec % min_tick == 0) {
            painter.drawLine(QPointF(x_tick, axisY + 1),
                QPointF(x_tick, axisY + 5));
        }
        if (cur_sec % max_tick == 0 || (min_range && cur_sec % 1 == 0)) {
            painter.drawLine(QPointF(x_tick, axisY + 1),
                QPointF(x_tick, axisY + 8));
            QString tick_text = formatDuration(cur_sec); // 例如 "0:30"
            int tw = fm.horizontalAdvance(tick_text);
            float x_text = x_tick - tw * 0.5f;
            float y_text = axisY + fm.height() + 2; // 軸線下方
            painter.drawText(QPointF(x_text, y_text), tick_text);
        }
    }
}

void WaveformWidget::drawDuration(QPainter& painter, const QRect& rect) {
    if (cursor_ms_ < 0.f) {
        return;
    }

    // 1) 計算播放線的 X 座標
    float cur_sec = cursor_ms_ / 1000.f;
    float x_cursor = timeToX(cur_sec, rect);

    // 2) 先畫播放線
    painter.setPen(QPen(QColor(100, 200, 255), 1));
    painter.drawLine(QPointF(x_cursor, rect.top() + 1),
        QPointF(x_cursor, rect.bottom()));

    // 3) 取得顯示在時間框裡的文字(目前播放時間)
    QString time_text = formatDuration(cur_sec, false);

    // 4) 計算文字大小
    const QFontMetrics fm(painter.font());
    int text_width = fm.horizontalAdvance(time_text);
    int text_height = fm.height();

    // 5) 時間框的寬高(含內部 padding)
    float box_w = text_width + kPadding * 2;
    float box_h = text_height + kPadding * 2;

    // 6) 預設放在播放線「右側」並留1px距離
    float lineGap = 1.0f;
    float x_text = x_cursor + lineGap;

    // 若「右側」放不下(會超過 rect.right())，改畫在播放線「左側」並同樣留1px
    if (x_text + box_w > rect.right()) {
        x_text = x_cursor - lineGap - box_w;
    }

    // 若又超出左邊界，則 clamp 回到 rect.left()
    if (x_text < rect.left()) {
        x_text = rect.left();
    }

    // 7) 設定時間框的 Y 座標：這裡示範放在 rect.top() + 3.f
    float y_box = rect.top() + 3.f;

    // 背景框
    QRectF bg_rect(x_text, y_box, box_w, box_h);

    // 8) 畫背景(帶圓角)
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(50, 50, 200, 200));
    painter.drawRoundedRect(bg_rect, kCornerRadius, kCornerRadius);

    // 9) 畫文字（讓文字置於框內，baseline 適度往下）
    painter.setPen(Qt::white);
    float text_baseline_y = y_box + kPadding + (text_height - fm.descent());
    float text_start_x = x_text + kPadding;
    painter.drawText(QPointF(text_start_x, text_baseline_y), time_text);
}

void WaveformWidget::drawFrequencyAxis(QPainter& painter, const QRect& rect) {
    // 頻率軸畫在 rect 左邊的 margin 區域
   // x 範圍是 [0, rect.left()]
    int axisX = 0;
    int axisWidth = rect.left();

    // 先畫一條垂直線：top ~ bottom
    painter.setPen(QPen(Qt::white, 1));
    //painter.drawLine(axisX + axisWidth - 1,
    //    rect.top(),
    //    axisX + axisWidth - 1,
    //    rect.bottom());

    // 預設 nyquist 頻率
    float nyquist = static_cast<float>(sample_rate_) * 0.5f;
    if (nyquist <= 0.f) return;

    QFont f = painter.font();
    f.setPointSize(8);
    painter.setFont(f);
    QFontMetrics fm(painter.font());

    // 每 5 kHz 畫一次刻度(僅示意)
    constexpr float kStepKHz = 5000.f;
    for (float freq = 0.f; freq <= nyquist + 1.f; freq += kStepKHz) {
        float ratio = freq / nyquist; // [0..1]
        if (ratio > 1.f) ratio = 1.f;

        // 軸上對應的 y 座標 = rect.bottom() - ratio * rect.height()
        float y_tick = rect.bottom() - ratio * rect.height();

        // 刻度線
        painter.drawLine(QPointF(axisWidth - 6, y_tick),
            QPointF(axisWidth, y_tick));

        // 文字
        QString label = QString::number(static_cast<int>(freq * 0.001f)) + " kHz"_str;
        int text_width = fm.horizontalAdvance(label);
        // 畫在刻度左方 (或更左)
        painter.drawText(axisWidth - 8 - text_width, y_tick + fm.height() * 0.4f, label);
    }
}

void WaveformWidget::paintEvent(QPaintEvent *event) {
    QPainter painter(this);

    painter.setRenderHints(QPainter::Antialiasing 
        | QPainter::SmoothPixmapTransform 
        | QPainter::TextAntialiasing);

    painter.setClipRegion(event->region());

	QRect waveformRect = drawRect();

    QPen framePen(Qt::white, 1);
    painter.setPen(framePen);
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(waveformRect);

    if (draw_mode_ & DRAW_SPECTROGRAM) {
        if (!spectrogram_cache_.isNull()) {
            painter.drawPixmap(waveformRect, spectrogram_cache_);
            drawFrequencyAxis(painter, waveformRect);
        }
    } else {
        painter.drawPixmap(waveformRect, cache_);
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

    drawDuration(painter, waveformRect);
    drawTimeAxis(painter, waveformRect);
}

float WaveformWidget::xToTime(float x, const QRect& rect) const {
    if (rect.width() <= 0) return 0.f;
    float total_sec = total_ms_ / 1000.f;
    if (total_sec <= 0.f) return 0.f;

    // 先算在 waveformRect 的比例
    float localX = x - rect.left();
    if (localX < 0.f) localX = 0.f;
    if (localX > rect.width()) localX = static_cast<float>(rect.width());

    float ratio = localX / static_cast<float>(rect.width());
    float sec = ratio * total_sec;
    return sec;
}

float WaveformWidget::timeToX(float sec, const QRect& rect) const {
    if (total_ms_ <= 0.f) return static_cast<float>(rect.left());
    float total_sec = total_ms_ / 1000.f;
    if (total_sec <= 0.f) return static_cast<float>(rect.left());

    float ratio = sec / total_sec; // [0..1]
    if (ratio < 0.f) ratio = 0.f;
    if (ratio > 1.f) ratio = 1.f;

    return rect.left() + ratio * rect.width();
}

float WaveformWidget::mapFreqToY(float freq, const QRect& rect) const {
    float nyquist = static_cast<float>(sample_rate_) * 0.5f;
    if (nyquist <= 0.f) {
        return static_cast<float>(rect.bottom());
    }
    float ratio = freq / nyquist; // [0..1]
    if (ratio < 0.f) ratio = 0.f;
    if (ratio > 1.f) ratio = 1.f;

    // 頻率越高，y 越靠上 (0 在頂端)
    float y = rect.bottom() - ratio * rect.height();
    return y;
}

void WaveformWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        QRect waveformRect = drawRect();
        float x = static_cast<float>(event->pos().x());
        float sec = xToTime(x, waveformRect);
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
