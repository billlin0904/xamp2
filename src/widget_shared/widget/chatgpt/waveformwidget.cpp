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
    void GetChannelPeaksAndRmsSimdAVX2(const std::vector<float>& buffer,
        size_t frame_per_peek,
        std::vector<float>& left_peaks,
        std::vector<float>& right_peaks,
        std::vector<float>& left_rms,
        std::vector<float>& right_rms)
    {
        // 若資料量不足（至少要有 L,R）
        if (buffer.size() < 2) {
            return;
        }

        // 為避免多次 realloc，可先 reserve
        if (left_peaks.empty()) {
            left_peaks.reserve(frame_per_peek * 8);
            left_rms.reserve(frame_per_peek * 8);
        }
        if (right_peaks.empty()) {
            right_peaks.reserve(frame_per_peek * 8);
            right_rms.reserve(frame_per_peek * 8);
        }

        // buffer.size() 為 float 數量，每 2 個 float => (L, R) 一組
        const size_t frame_count = buffer.size() / 2;

        // 這個 mask 用來取絕對值：相當於把符號位清掉 (x & 0x7FFFFFFF)
        const __m256 sign_mask = _mm256_castsi256_ps(_mm256_set1_epi32(0x7FFFFFFF));

        // gather 時用的「左右聲道 offset」，一次 gather 8 個
        // 例如 left_indices: [0,2,4,6, 8,10,12,14] 表示從 interleaved (L,R) 抓 L
        // right_indices: [1,3,5,7, 9,11,13,15] 表示抓 R
        alignas(32) static const int left_offset[8] = { 0, 2, 4,  6,  8, 10, 12, 14 };
        alignas(32) static const int right_offset[8] = { 1, 3, 5,  7,  9, 11, 13, 15 };

        for (size_t start = 0; start < frame_count; start += frame_per_peek) {
            // end 為本 chunk 範圍
            size_t end = std::min(start + frame_per_peek, frame_count);
            size_t count = end - start;
            if (count == 0) {
                continue;
            }

            // 用來累計 max 與 sum of squares 的 SIMD 累加器
            __m256 max_left_vec = _mm256_setzero_ps();
            __m256 max_right_vec = _mm256_setzero_ps();
            __m256 sum_sq_left_vec = _mm256_setzero_ps();
            __m256 sum_sq_right_vec = _mm256_setzero_ps();

            // SIMD 分批處理(每次 8 個 frame)
            // chunk 的起始位置是 start，結束是 end
            // 我們一次前進 8 frame
            size_t simd_count = (count / 8) * 8; // 可被 8 整除的部分
            size_t i = 0; // 相對於 chunk 的偏移
            for (; i < simd_count; i += 8) {
                // base: 在整個 buffer 中的 index => (start + i) * 2 (每 frame 2 float)
                int base = static_cast<int>((start + i) * 2);

                // gather 左聲道
                __m256i left_indices = _mm256_add_epi32(
                    _mm256_set1_epi32(base),
                    _mm256_load_si256(reinterpret_cast<const __m256i*>(left_offset))
                );
                __m256 leftVal = _mm256_i32gather_ps(buffer.data(), left_indices, 4);
                leftVal = _mm256_and_ps(leftVal, sign_mask); // abs

                // gather 右聲道
                __m256i right_indices = _mm256_add_epi32(
                    _mm256_set1_epi32(base),
                    _mm256_load_si256(reinterpret_cast<const __m256i*>(right_offset))
                );
                __m256 rightVal = _mm256_i32gather_ps(buffer.data(), right_indices, 4);
                rightVal = _mm256_and_ps(rightVal, sign_mask); // abs

                // peak => 取最大值
                max_left_vec = _mm256_max_ps(max_left_vec, leftVal);
                max_right_vec = _mm256_max_ps(max_right_vec, rightVal);

                // sum of squares => 累加 (x^2)
                __m256 left_sq = _mm256_mul_ps(leftVal, leftVal);
                __m256 right_sq = _mm256_mul_ps(rightVal, rightVal);
                sum_sq_left_vec = _mm256_add_ps(sum_sq_left_vec, left_sq);
                sum_sq_right_vec = _mm256_add_ps(sum_sq_right_vec, right_sq);
            }

            // 針對剩餘 (count % 8 != 0) 的部分，用 scalar 方式處理
            for (; i < count; i++) {
                float L = std::fabs(buffer[(start + i) * 2 + 0]);
                float R = std::fabs(buffer[(start + i) * 2 + 1]);

                // peak
                // 先把 SIMD accumulator 的前 1 個元素取出比對也可以
                // 這裡簡單另外用暫存 scalar
                if (L > reinterpret_cast<float*>(&max_left_vec)[0]) {
                    reinterpret_cast<float*>(&max_left_vec)[0] = L;
                }
                if (R > reinterpret_cast<float*>(&max_right_vec)[0]) {
                    reinterpret_cast<float*>(&max_right_vec)[0] = R;
                }

                // sum of squares
                reinterpret_cast<float*>(&sum_sq_left_vec)[0] += (L * L);
                reinterpret_cast<float*>(&sum_sq_right_vec)[0] += (R * R);
            }

            // 現在要「水平化」(horizontal reduce) 這四個 __m256
            // = 找出其 8 個元素的最大值 / 加總
            auto hmax = [&](__m256 v) {
                // 簡易做法：先存到陣列，用 std::max_element
                alignas(32) float tmp[8];
                _mm256_store_ps(tmp, v);
                float m = tmp[0];
                for (int k = 1; k < 8; k++) {
                    if (tmp[k] > m) m = tmp[k];
                }
                return m;
                };
            auto hsum = [&](__m256 v) {
                alignas(32) float tmp[8];
                _mm256_store_ps(tmp, v);
                float s = 0.f;
                for (int k = 0; k < 8; k++) {
                    s += tmp[k];
                }
                return s;
                };

            float max_left = hmax(max_left_vec);
            float max_right = hmax(max_right_vec);
            float sum_sq_left = hsum(sum_sq_left_vec);
            float sum_sq_right = hsum(sum_sq_right_vec);

            // 再確認上面 scalar remainder 部分若有更新到 [0] 位置，也要一起比較
            // （本範例直接就比較了，所以無需再特別合併）

            // 最終 RMS
            float rms_left = 0.f;
            float rms_right = 0.f;
            if (count > 0) {
                rms_left = std::sqrt(sum_sq_left / (float)count);
                rms_right = std::sqrt(sum_sq_right / (float)count);
            }

            // 寫回結果
            left_peaks.push_back(max_left);
            right_peaks.push_back(max_right);
            left_rms.push_back(rms_left);
            right_rms.push_back(rms_right);
        }
    }

    constexpr size_t kFFTSize = 4096 * 2;
    constexpr size_t kHopSize = kFFTSize * 0.5;

    auto makeImage(double duration_sec, uint32_t sample_rate, const QImage& chunk) -> QImage {
        size_t max_time_bins = static_cast<size_t>(std::ceil(
            duration_sec
            * sample_rate / kHopSize)) + 1;

        QSize image_size(max_time_bins, chunk.height());
        QImage spec_img(image_size, QImage::Format_RGB888);
        spec_img.fill(Qt::black);
        return spec_img;
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

		auto* show_spectrogram_act = show_ch_menu->addAction(
            tr("Show Spectrogram"), [this]() {
			setDrawMode(kDrawSpectrogram);
			});
        if (setting_value & kDrawSpectrogram) {
            show_spectrogram_act->setChecked(true);
        }

        auto* only_right_ch_act = show_ch_menu->addAction(
            tr("Show Only Right Channel"), [this]() {
            auto enable = draw_mode_ & kDrawPlayedArea;
            setDrawMode(kDrawOnlyRightChannel | (enable ? kDrawPlayedArea : 0));
            });
		if (setting_value & kDrawOnlyRightChannel) {
            only_right_ch_act->setChecked(true);
		}

        auto* only_left_ch_act = show_ch_menu->addAction(
            tr("Show Only Left Channel"), [this]() {
            auto enable = draw_mode_& kDrawPlayedArea;
            setDrawMode(kDrawOnlyLeftChannel | (enable ? kDrawPlayedArea : 0));
            });
        if (setting_value & kDrawOnlyLeftChannel) {
            only_left_ch_act->setChecked(true);
        }

        auto* only_both_ch_act = show_ch_menu->addAction(
            tr("Show Both Channel"), [this]() {
                auto enable = draw_mode_ & kDrawPlayedArea;
                setDrawMode(kDrawBothChannel | (enable ? kDrawPlayedArea : 0));
            });
        if (setting_value & kDrawBothChannel) {
            only_both_ch_act->setChecked(true);
        }

		show_ch_menu->addSeparator();

        auto* only_right_ch_rms_act = show_ch_menu->addAction(
            tr("Show Only Right Channel RMS"), [this]() {
                auto enable = draw_mode_ & kDrawPlayedArea;
                setDrawMode(kDrawOnlyRightChRms | (enable ? kDrawPlayedArea : 0));
            });
        if (setting_value & kDrawOnlyRightChRms) {
            only_right_ch_rms_act->setChecked(true);
        }

        auto* only_left_ch_rms_act = show_ch_menu->addAction(
            tr("Show Only Left Channel RMS"), [this]() {
                auto enable = draw_mode_ & kDrawPlayedArea;
                setDrawMode(kDrawOnlyLeftChRms | (enable ? kDrawPlayedArea : 0));
            });
        if (setting_value & kDrawOnlyLeftChRms) {
            only_left_ch_rms_act->setChecked(true);
        }

        auto* only_both_ch_rms_act = show_ch_menu->addAction(
            tr("Show Both Channel RMS"), [this]() {
            auto enable = draw_mode_ & kDrawPlayedArea;
            setDrawMode(kDrawBothChannelRms | (enable ? kDrawPlayedArea : 0));
            });
        if (setting_value & kDrawBothChannelRms) {
            only_both_ch_rms_act->setChecked(true);
        }

        auto submenu = action_map.addSubMenu(tr("Show Played Area"));
        auto* show_act = submenu->addAction(
            tr("Enable"), [this]() {
			uint32_t mode = 0;
			if (draw_mode_ & kDrawPlayedArea) {
                mode = draw_mode_ & ~kDrawPlayedArea;
			}
			else {
                mode = draw_mode_ | kDrawPlayedArea;
			}
			setDrawMode(mode);
            });
        if (setting_value & kDrawPlayedArea) {
            show_act->setChecked(true);
        }
        action_map.exec(pt);
        update();
        });
}

void WaveformWidget::setCurrentPosition(float sec) {
	cursor_ms_ = sec * 1000.f;

    if (left_peaks_.empty() || right_peaks_.empty()) {
        update(drawRect());
        return;
    }

    if (draw_mode_ & kDrawPlayedArea) {
        float ratio = cursor_ms_ / total_ms_;
        ratio = std::clamp(ratio, 0.f, 1.f);
        const int play_index = static_cast<int>(std::floor(ratio * peak_count_));
        updatePlayedPaths(play_index);
    }

    update(drawRect());    
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
	left_rms_.clear();
	right_rms_.clear();
	path_left_played_ = QPainterPath();
    path_right_played_ = QPainterPath();
	update();
}

void WaveformWidget::setDrawMode(uint32_t mode) {
    if (draw_mode_ & kDrawSpectrogram) {
        if (spectrogram_.isNull()) {
            emit readAudioSpectrogram(size(), file_path_);
        }
    } else {
        if (left_peaks_.empty() || right_peaks_.empty()) {
            emit readWaveformAudioData(frame_per_peak_, file_path_);
        }
    }
    draw_mode_ = mode;
    updateCachePixmap();
    qAppSettings.setValue(kAppSettingWaveformDrawMode, mode);
	update();
}

void WaveformWidget::setSpectrogramData(double duration_sec, const QImage& chunk, int time_index) {
    if (time_index == 0) {
        spectrogram_ = makeImage(duration_sec, sample_rate_, chunk);
        spectrogram_.fill(Qt::black);
    }

    QPainter p(&spectrogram_);
    p.drawImage(time_index, 0, chunk);
    p.end();

    updateSpectrogramSize();
    update();
}

void WaveformWidget::updateSpectrogramSize() {
    if (spectrogram_.isNull())
        return;
    if (size() == spectrogram_cache_.size()) {
        return;
    }
    spectrogram_cache_ = spectrogram_.scaled(size(),
        Qt::KeepAspectRatioByExpanding);
}

void WaveformWidget::onReadAudioData(const std::vector<float> & buffer) {
    //XAMP_LOG_DEBUG("Read audio data {} size frame_per_peak_: {}", buffer.size(), frame_per_peak_);
    GetChannelPeaksAndRmsSimdAVX2(buffer,
        frame_per_peak_,
        left_peaks_,
        right_peaks_, 
        left_rms_, 
        right_rms_);
    //XAMP_LOG_DEBUG("Update left_peaks size: {} right_peaks size: {}",
    //    left_peaks_.size(), right_peaks_.size());
    update();
}

QRect WaveformWidget::drawRect() const {
    constexpr int leftMargin = 50;
    constexpr int rightMargin = 20;
    constexpr int topMargin = 20;
    constexpr int bottomMargin = 20;

    QRect waveform_rect(leftMargin,
        topMargin,
        width() - leftMargin - rightMargin,
        height() - topMargin - bottomMargin);

    return waveform_rect;
}

void WaveformWidget::drawCursorIfNeeded(QPainter& painter, const QRegion& region) {
    if (cursor_ms_ < 0.f || total_ms_ <= 0.f) {
        return;  // 無需繪製
    }

    QRect waveformRect = drawRect();

    // 計算cursor的x座標位置
    float cursor_sec = cursor_ms_ / 1000.f;
    float x_cursor = timeToX(cursor_sec, waveformRect);
    int cursor_x = static_cast<int>(x_cursor);

    // 檢查重繪區域是否包含cursor的位置
    QRect cursorRect(cursor_x - 1, waveformRect.top(), 2, waveformRect.height());
    if (!region.intersects(cursorRect)) {
        return; // 若不相交，則不繪製
    }

    // 繪製cursor線
    painter.save();

    // 繪製cursor（可自訂樣式）
    painter.setPen(QPen(QColor(100, 200, 255), 1));
    painter.drawLine(QPointF(x_cursor, waveformRect.top()),
        QPointF(x_cursor, waveformRect.bottom()));

    // 繪製時間提示標籤
    QString time_text = formatDuration(cursor_sec, false);

    const QFontMetrics fm(painter.font());
    int text_width = fm.horizontalAdvance(time_text);
    int text_height = fm.height();

    constexpr float kPadding = 4.0f;
    constexpr float kCornerRadius = 3.0f;

    float box_w = text_width + kPadding * 2;
    float box_h = text_height + kPadding * 2;

    // 計算標籤位置，避免超出區域
    float lineGap = 1.0f;
    float x_text = x_cursor + lineGap;

    if (x_text + box_w > waveformRect.right()) {
        x_text = x_cursor - lineGap - box_w;
    }

    if (x_text < waveformRect.left()) {
        x_text = waveformRect.left();
    }

    float y_box = waveformRect.top() + 3.f;

    QRectF bg_rect(x_text, y_box, box_w, box_h);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(50, 50, 200, 200));
    painter.drawRoundedRect(bg_rect, kCornerRadius, kCornerRadius);

    painter.setPen(Qt::white);
    float text_baseline_y = y_box + kPadding + (text_height - fm.descent());
    float text_start_x = x_text + kPadding;
    painter.drawText(QPointF(text_start_x, text_baseline_y), time_text);

    painter.restore();
}

void WaveformWidget::updateCachePixmap() {
    if (left_peaks_.empty() || right_peaks_.empty()) {
        return;
    }

    QRect waveform_rect = drawRect();
    if (waveform_rect.width() <= 0 || waveform_rect.height() <= 0) {
        return;
    }

    XAMP_LOG_DEBUG("left_peaks size:{} right_peaks size:{}",
        left_peaks_.size(),
        right_peaks_.size());

    QImage img(waveform_rect.size(), QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);

    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    const int pc = static_cast<int>(left_peaks_.size());
    if (pc < 1) return;

    auto draw_one_channel = [&](const std::vector<float>& peaks,
        const QColor& color,
        int topInRect,
        int channelHeight) {
        QPainterPath path;

        auto mapIndexToLocalX = [&](int i) {
            if (pc <= 1) return 0.f;
            return static_cast<float>(i) / (pc - 1)
                * static_cast<float>(waveform_rect.width() - 1);
            };

        float x0 = mapIndexToLocalX(0);
        float y0 = mapPeakToY(peaks[0],
            topInRect, 
            channelHeight,
            true);
        path.moveTo(x0, y0);

        for (int i = 1; i < pc; i++) {
            float x = mapIndexToLocalX(i);
            float y = mapPeakToY(peaks[i],
                topInRect, 
                channelHeight, 
                true);
            path.lineTo(x, y);
        }

        for (int i = pc - 1; i >= 0; i--) {
            float x = mapIndexToLocalX(i);
            float y = mapPeakToY(peaks[i],
                topInRect, 
                channelHeight,
                false);
            path.lineTo(x, y);
        }
        path.closeSubpath();

        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawPath(path);
        };

    auto draw_rms_curve = [&](const std::vector<float>& rms,
        const QColor& fillColor,
        int topInRect,
        int channelHeight) {
        // 如果沒有 RMS 資料就不畫
        if (rms.empty()) return;

        QPainterPath rmsPath;

        // 根據需求，把「i -> x 座標」的函式放在這裡
        auto mapIndexToLocalX = [&](int i) -> float {
            if (pc <= 1) return 0.0f;
            // 下例假設 waveform_rect.width() - 1 可做為繪圖寬度
            // 視你的程式結構自行修改
            return static_cast<float>(i) / (pc - 1) * (waveform_rect.width() - 1);
            };

        // 先計算此聲道波形「中央線」的位置
        float midY = topInRect + channelHeight * 0.5f;
        float amplitudeRange = (channelHeight * 0.5f) * kHeadroomFactor;

        // 1) 移動到左邊 (索引0) 的「上半」起始點
        float x0 = mapIndexToLocalX(0);
        float y0_up = midY - (rms[0] * amplitudeRange);
        rmsPath.moveTo(x0, y0_up);

        // 2) 從左到右，畫「上半」(upper side)
        for (int i = 1; i < pc; ++i) {
            float x = mapIndexToLocalX(i);
            float y_up = midY - (rms[i] * amplitudeRange);
            rmsPath.lineTo(x, y_up);
        }

        // 3) 再反向從右到左，畫「下半」(lower side)
        //    這樣就能把整個區域封閉起來
        for (int i = pc - 1; i >= 0; --i) {
            float x = mapIndexToLocalX(i);
            float y_down = midY + (rms[i] * amplitudeRange);
            rmsPath.lineTo(x, y_down);
        }

        // 4) 收尾關閉路徑
        rmsPath.closeSubpath();

        // 5) 設定無邊框、用指定顏色填滿
        p.setPen(Qt::NoPen);
        p.setBrush(fillColor);
        p.drawPath(rmsPath);
        };

    int h = waveform_rect.height();
    if (draw_mode_ & kDrawBothChannel) {
        draw_one_channel(left_peaks_, 
            kLeftUnPlayedChannelColor,
            0,
            h / 2);
        draw_one_channel(right_peaks_,
            kRightUnPlayedChannelColor, 
            h / 2, 
            h / 2);

        draw_rms_curve(left_rms_,
            kRmsColor,
            0,
            h / 2);
        draw_rms_curve(right_rms_,
            kRmsColor,
            h / 2,
            h / 2);
    }
    else if (draw_mode_ & kDrawOnlyLeftChannel) {
        draw_one_channel(left_peaks_,
            kLeftUnPlayedChannelColor,
            0,
            h);

        draw_rms_curve(left_rms_,
            kRmsColor,
            0,
            h);
    }
    else if (draw_mode_ & kDrawOnlyRightChannel) {
        draw_one_channel(right_peaks_,
            kRightUnPlayedChannelColor,
            0,
            h);

        draw_rms_curve(right_rms_,
            kRmsColor,
            0,
            h);
    }
    else if (draw_mode_ & kDrawOnlyRightChRms) {
        draw_rms_curve(left_rms_,
            kRmsColor,
            0,
            h);
	}
	else if (draw_mode_ & kDrawOnlyLeftChRms) {
        draw_rms_curve(left_rms_,
            kRmsColor,
            0,
            h);
	}
	else if (draw_mode_ & kDrawBothChannelRms) {
        draw_rms_curve(left_rms_,
            kRmsColor,
            0,
            h / 2);
        draw_rms_curve(right_rms_,
            kRmsColor,
            h / 2,
            h / 2);
	}

    p.end();

    cache_ = QPixmap::fromImage(img);

    peak_count_ = pc;
    total_ms_ = static_cast<float>(peak_count_)
        * (frame_per_peak_ * (1000.f / static_cast<float>(sample_rate_)));
}

void WaveformWidget::updatePlayedPaths(int playIndex) {
    if (playIndex <= 0) {
        path_left_played_ = QPainterPath();
        path_right_played_ = QPainterPath();
        return;
    }

    if (playIndex > static_cast<int>(left_peaks_.size())) {
        playIndex = static_cast<int>(left_peaks_.size());
    }

    QRect waveform_rect = drawRect();

    if (waveform_rect.width() <= 0 || waveform_rect.height() <= 0) {
        path_left_played_ = QPainterPath();
        path_right_played_ = QPainterPath();
        path_left_rms_played_ = QPainterPath();
        path_right_rms_played_ = QPainterPath();
        return;
    }

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

            auto mapIndexToAbsX = [&](int i) {
                if (total_count <= 1) return static_cast<float>(waveform_rect.left());
                float ratio = static_cast<float>(i) / (total_count - 1);
                float xRange = static_cast<float>(waveform_rect.width() - 1);
                return waveform_rect.left() + ratio * xRange;
                };

            float x0 = mapIndexToAbsX(startIndex);
            float y0 = waveform_rect.top() + mapPeakToY(peaks[startIndex],
                topInRect,
                channelHeight,
                true);
            path.moveTo(x0, y0);

            for (int i = startIndex + 1; i < endIndex; ++i) {
                float x = mapIndexToAbsX(i);
                float y = waveform_rect.top() + mapPeakToY(peaks[i],
                    topInRect,
                    channelHeight,
                    true);
                path.lineTo(x, y);
            }

            // 負半
            for (int i = endIndex - 1; i >= startIndex; --i) {
                float x = mapIndexToAbsX(i);
                float y = waveform_rect.top() + mapPeakToY(peaks[i],
                    topInRect,
                    channelHeight,
                    false);
                path.lineTo(x, y);
            }
            path.closeSubpath();
            return path;
        };

    int h = waveform_rect.height();
    if (draw_mode_ & kDrawBothChannel) {
        path_left_played_ = buildChannelPath(left_peaks_,
            0,
            playIndex,
            0, 
            h / 2);
        path_right_played_ = buildChannelPath(right_peaks_,
            0,
            playIndex,
            h / 2, 
            h / 2);
    }
    else if (draw_mode_ & kDrawOnlyLeftChannel) {
        path_left_played_ = buildChannelPath(left_peaks_,
            0,
            playIndex,
            0,
            h);
        path_right_played_ = QPainterPath();
    }
    else if (draw_mode_ & kDrawOnlyRightChannel) {
        path_left_played_ = QPainterPath();
        path_right_played_ = buildChannelPath(right_peaks_,
            0,
            playIndex,
            0,
            h);
    }

    int rmsCount = (int)left_rms_.size();

    if (playIndex > rmsCount) {
        playIndex = rmsCount;
    }

    path_left_rms_played_ = QPainterPath();
    path_right_rms_played_ = QPainterPath();

    if (rmsCount > 0 && playIndex > 0) {
        if (draw_mode_ & kDrawBothChannelRms) {
            path_left_rms_played_ = buildChannelPath(left_rms_,
                0, playIndex, 0, h / 2);
            path_right_rms_played_ = buildChannelPath(right_rms_,
                0, playIndex, h / 2, h / 2);
        }
        else if (draw_mode_ & kDrawOnlyLeftChRms) {
            path_left_rms_played_ = buildChannelPath(left_rms_, 
                0, playIndex, 0, h);
        }
        else if (draw_mode_ & kDrawOnlyRightChRms) {
            path_right_rms_played_ = buildChannelPath(right_rms_,
                0, playIndex, 0, h);
        }
    }
}

void WaveformWidget::drawTimeAxis(QPainter& painter, const QRect& rect) {
    // 1) 取得音訊總秒數 (int)
    int total_sec = static_cast<int>(total_ms_ / 1000.f);
    if (total_sec <= 0) return;

    // 2) 設定字型、計算標籤寬度
    //    Spek 通常用 "00:00" 作為範例字串，測量文字長度。
    QFont f = painter.font();
    f.setPointSize(7); // 你可以調整字型大小
    painter.setFont(f);
    QFontMetrics fm(painter.font());

    // 假設我們用 "00:00" 來估計字串長度
    QString sample_label = "00:00"_str;
    int labelWidth = fm.horizontalAdvance(sample_label);
    // 給個倍數，確保標籤之間留足空間 (Spek 內用 ~1.5)
    double spacingFactor = 1.5;

    // 3) 準備 Spek 狀態下常用的 time factor (秒)
    //    這組清單與 Spek 幾乎一致: 
    //    1,2,5,10,20,30 秒、1分(60s)、2分(120s)、5分(300s)...等
    static const int time_factors[] = {
        1,2,5,10,20,30,
        60,120,300,600,1200,1800, // 1m,2m,5m,10m,20m,30m
        // 可再加更大, 3600(1h),7200(2h) ...
        0 // 結束標記
    };

    // 4) 計算每「1 秒」對應多少像素
    double scale = 0.0;
    if (total_sec > 0) {
        scale = double(rect.width()) / double(total_sec);
    }

    // 5) 從候選因子中選擇「最小可行」間距
    //    條件：相鄰標籤間距 >= spacingFactor * labelWidth
    //    => scale * factor >= spacingFactor * labelWidth
    //    => factor >= (spacingFactor * labelWidth) / scale
    int factor = 0;
    for (int i = 0; time_factors[i] != 0; ++i) {
        double pixelPerTick = scale * time_factors[i]; // 該刻度間距對應的畫布寬度
        if (pixelPerTick >= spacingFactor * labelWidth) {
            factor = time_factors[i];
            break;
        }
    }
    // 若都找不到 => total_sec 很大 => 就取最後1個大的 (例如 30分或1小時)
    if (factor == 0) {
        factor = 1800; // 你可改為更大的值，如 3600
    }

    // 6) 準備繪製：包含 0 與終點
    int min_units = 0;
    int max_units = total_sec;

    // 7) 開始畫軸
    int axisY = rect.bottom();
    painter.setPen(QPen(Qt::white, 1));

    // 先畫最左的 0秒 及最右的 max_units
    auto drawTickAndLabel = [&](int t) {
        // t秒 => x座標
        double ratio = double(t) / double(total_sec);
        if (ratio > 1.0) ratio = 1.0;
        float x_tick = float(rect.left() + ratio * rect.width());
        // 刻度線
        painter.drawLine(QPointF(x_tick, axisY),
            QPointF(x_tick, axisY + 4));
        // 標籤 mm:ss
        QString label = formatDuration(t);
        int tw = fm.horizontalAdvance(label);
        float x_text = x_tick - tw * 0.5f;
        float y_text = axisY + fm.height() + 2;
        painter.drawText(QPointF(x_text, y_text), label);
        };

    // 畫 0
    drawTickAndLabel(min_units);

    // 8) 中間刻度（factor倍數）。Spek 如果跟終點太近 => 跳出避免重疊
    for (int tick = min_units + factor; tick < max_units; tick += factor) {
        // 檢查是否離終點太近
        // => (max_units - tick)秒 * scale < labelWidth * 1.2
        double distToEndPx = (max_units - tick) * scale;
        if (distToEndPx < double(labelWidth) * 1.2) {
            break;
        }
        drawTickAndLabel(tick);
    }

    // 畫終點
    drawTickAndLabel(max_units);
}

void WaveformWidget::drawDuration(QPainter& painter, const QRect& rect) {
    if (cursor_ms_ < 0.f) {
        return;
    }

    float cur_sec = cursor_ms_ / 1000.f;
    float x_cursor = timeToX(cur_sec, rect);

    painter.setPen(QPen(QColor(100, 200, 255), 1));
    painter.drawLine(QPointF(x_cursor, rect.top() + 1),
        QPointF(x_cursor, rect.bottom()));

    QString time_text = formatDuration(cur_sec, false);

    const QFontMetrics fm(painter.font());
    int text_width = fm.horizontalAdvance(time_text);
    int text_height = fm.height();

    float box_w = text_width + kPadding * 2;
    float box_h = text_height + kPadding * 2;

    float lineGap = 1.0f;
    float x_text = x_cursor + lineGap;

    if (x_text + box_w > rect.right()) {
        x_text = x_cursor - lineGap - box_w;
    }

    if (x_text < rect.left()) {
        x_text = rect.left();
    }

    float y_box = rect.top() + 3.f;

    QRectF bg_rect(x_text, y_box, box_w, box_h);

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
    // 1) 計算音檔的 Nyquist 頻率 = 取樣率的一半
    double nyquist = double(sample_rate_) * 0.5;
    if (nyquist <= 0.0) return;

    // 2) 設定字型，測量標籤寬度
    QFont f = painter.font();
    f.setPointSize(7); // 調整字型大小
    painter.setFont(f);
    QFontMetrics fm(painter.font());

    // Spek 可能用 "22.0 kHz" 當範例字串 => "22.0 kHz"
    // 你也可用 "99999 Hz" 之類，以確保最寬
    QString sample_label = "22.0 kHz"_str;
    int labelWidth = fm.horizontalAdvance(sample_label);
    double spacingFactor = 1.5; // 允許標籤間保留 1.5 個字寬

    // 3) 準備頻率刻度候選 (比時間多/大，從 1Hz ~ 20000Hz 之類)
    //    下例為簡化示例，可再增刪
    static const int freq_factors[] = {
        // 1,2,5,10,20,50,100,200,500, 
        1000,2000,3000,5000,10000,20000,
        0
    };
    // 你也可從較小開始: 1,2,5,10,20,50,100,200,500,1000,2000...
    // 若要更細，如 200, 400, 800... 可依需求添加

    // 4) 計算「1Hz 在畫面上佔幾個像素」
    //    這裡是「垂直軸」；所以 1Hz = rect.height() / nyquist
    //    取 linear scale => y = bottom - (freq/nyquist)*height
    double scale = 0.0;
    if (nyquist > 0.0) {
        scale = double(rect.height()) / nyquist;
    }

    // 5) 選擇「最小可行」刻度值 (freq step)
    int factor = 0;
    for (int i = 0; freq_factors[i] != 0; ++i) {
        double pixelPerTick = scale * freq_factors[i];
        if (pixelPerTick >= spacingFactor * labelWidth) {
            factor = freq_factors[i];
            break;
        }
    }
    // 如果一個都找不到 => 代表檔案取樣率極高 => 就取最後(20000)
    if (factor == 0) {
        factor = 20000;
    }

    // 6) 頻率軸通常畫在「rect.left()」左邊一點
    //    先畫一條主線
    const int axisRight = rect.left();
    painter.setPen(Qt::white);

    // 7) 幫你寫個小函式，用來繪製單一 tick
    auto drawTickAndLabel = [&](double freq) {
        if (freq < 0.0) freq = 0.0;
        if (freq > nyquist) freq = nyquist;

        double ratio = freq / nyquist;
        if (ratio > 1.0) ratio = 1.0;
        float y_tick = float(rect.bottom() - ratio * rect.height());

        // 繪製小刻度
        painter.drawLine(QPointF(axisRight - 5, y_tick),
            QPointF(axisRight, y_tick));

        // 顯示文字: e.g. "2.0 kHz"
        // freq 若 < 1000 => 顯示 "123 Hz"
        // freq >= 1000 => 顯示 kHz
        QString label;
        if (freq < 999.5) {
            label = QString::number(freq, 'f', 0) + " Hz"_str;
        }
        else {
            double khz = freq * 1e-3;
            label = QString::number(khz, 'f', 1) + " kHz"_str;
        }
        int tw = fm.horizontalAdvance(label);
        float y_text = y_tick + fm.ascent() * 0.5f;

        // 左對齊
        painter.drawText(axisRight - 8 - tw, y_text, label);
        };

    // 8) 先畫0, 再畫中間刻度, 最後畫 nyquist
    drawTickAndLabel(0.0);

    // 中間刻度
    for (int tick = factor; tick < int(nyquist); tick += factor) {
        // Spek 會檢查是否離nyquist 太近 => 跳過 
        double distToEndPx = (nyquist - tick) * scale;
        if (distToEndPx < labelWidth * 1.2) {
            break;
        }
        drawTickAndLabel(double(tick));
    }

    // 再畫 nyquist
    drawTickAndLabel(nyquist);
}

void WaveformWidget::paintEvent(QPaintEvent *event) {
    QPainter painter(this);

    painter.setRenderHints(QPainter::Antialiasing 
        | QPainter::SmoothPixmapTransform 
        | QPainter::TextAntialiasing);

    painter.setClipRegion(event->region());

    const QRect waveform_rect = drawRect();

    QPen framePen(Qt::white, 1);
    painter.setPen(framePen);
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(waveform_rect);

    if (draw_mode_ & kDrawSpectrogram) {
        if (!spectrogram_cache_.isNull()) {
            painter.drawImage(waveform_rect, spectrogram_cache_);
            drawFrequencyAxis(painter, waveform_rect);
        }
    } else {
        painter.drawPixmap(waveform_rect, cache_);
    }

	if (draw_mode_ & kDrawPlayedArea) {
        if (draw_mode_ & kDrawBothChannel 
            || draw_mode_ & kDrawOnlyLeftChannel) {
			painter.setPen(Qt::NoPen);
			painter.setBrush(kLeftPlayedChannelColor);
			painter.drawPath(path_left_played_);
		}
		if (draw_mode_ & kDrawBothChannel 
            || draw_mode_ & kDrawOnlyRightChannel) {
			painter.setPen(Qt::NoPen);
			painter.setBrush(kRightPlayedChannelColor);
			painter.drawPath(path_right_played_);
		}
	}

    if (draw_mode_
        & (kDrawOnlyLeftChRms | kDrawOnlyRightChRms | kDrawBothChannelRms)) {
        painter.setPen(Qt::NoPen);
        QColor rmsLeftColor(0, 255, 128, 200);
        QColor rmsRightColor(0, 255, 255, 200);
        painter.setBrush(rmsLeftColor);
        painter.drawPath(path_left_rms_played_);

        painter.setBrush(rmsRightColor);
        painter.drawPath(path_right_rms_played_);
    }

    drawDuration(painter, waveform_rect);
    drawTimeAxis(painter, waveform_rect);
}

float WaveformWidget::xToTime(float x, const QRect& rect) const {
    if (rect.width() <= 0) return 0.f;
    float total_sec = total_ms_ / 1000.f;
    if (total_sec <= 0.f) return 0.f;

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
    if (draw_mode_ & kDrawSpectrogram) {
		if (file_path_.empty() || is_processing_) {
			return;
		}
        doneRead();
        is_processing_ = true;
        emit readAudioSpectrogram(size(), file_path_);
	} else {
        updateCachePixmap();
	}
	update();
}

void WaveformWidget::doneRead() {
    XAMP_LOG_DEBUG("Done read!");
    updateSpectrogramSize();
    updateCachePixmap();
    spectrogram_ = QImage();
    is_processing_ = false;
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
	left_rms_.clear();
	right_rms_.clear();
	path_left_played_ = QPainterPath();
    path_right_played_ = QPainterPath();
    spectrogram_ = QImage();
    spectrogram_cache_ = QImage();
    update();
}

void WaveformWidget::setProcessInfo(size_t frame_per_peek, const Path& file_path) {
	frame_per_peak_ = frame_per_peek;
	file_path_ = file_path;
    draw_mode_ = qAppSettings.valueAsInt(kAppSettingWaveformDrawMode);
	if (draw_mode_ & kDrawSpectrogram) {
		emit readAudioSpectrogram(size(), file_path_);
	}
	else {
		emit readWaveformAudioData(frame_per_peak_, file_path_);
	}
	update();
}

float WaveformWidget::mapPeakToY(float peakVal, 
    int top,
    int height, 
    bool isPositive) const {
    float mid_y = top + height * 0.5f;
    float amplitude_range = (height * 0.5f) * kHeadroomFactor;

    if (isPositive) {
        return mid_y - peakVal * amplitude_range;
    }
    else {
        return mid_y + peakVal * amplitude_range;
    }
}
