//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#include <widget/waveformslider.h>

#include <QMouseEvent>
#include <QPainter>
#include <QPropertyAnimation>
#include <QFutureWatcher>
#include <QtConcurrent>

#include <widget/util/read_util.h>

#include <algorithm>
#include <cmath>
#include <vector>

namespace {
    constexpr int kDefaultHeight = 32;
    constexpr int kMinimumWidth = 96;
    constexpr qreal kHorizontalPadding = 4.0;
    constexpr qreal kVerticalPadding = 4.0;
    constexpr qreal kCornerRadius = 4.0;
    constexpr qreal kWaveformBarWidth = 4.0;
    constexpr qreal kWaveformBarGap = 4.0;
    constexpr qreal kMinimumBarHeight = kWaveformBarWidth;
    constexpr float kWaveformRmsGain = 4.5f;

    float normalizedPeak(float value) {
        return std::clamp(std::abs(value), 0.0f, 1.0f);
    }

    struct WaveformReadResult {
        QVector<float> peaks;
        QString error_message;
    };

    WaveformReadResult readWaveformPeaks(const QString& file_path, int peak_count) {
        static constexpr uint32_t kReadFrames = 8192;

        WaveformReadResult result;
        try {
            if (file_path.isEmpty()) {
                return result;
            }

            const auto file_stream = makePcmFileStream(file_path.toStdWString(), 0.0f);
            const auto duration = file_stream->GetDuration();
            if (duration <= 0.0) {
                return result;
            }

            const auto format = AudioFormat::ToFloatFormat(file_stream->GetFormat());
            const auto channels = (std::max<uint16_t>)(1, format.GetChannels());
            const auto sample_rate = (std::max<uint32_t>)(1, format.GetSampleRate());
            const auto total_frames = (std::max<uint64_t>)(1,
                static_cast<uint64_t>(std::llround(duration * sample_rate)));
            const auto target_peak_count = (std::max)(1, peak_count);

            std::vector<double> bucket_square_sums(target_peak_count, 0.0);
            std::vector<uint32_t> bucket_counts(target_peak_count, 0);

            std::vector<float> buffer(kReadFrames * channels + 1024);
            uint64_t frame_index = 0;

            // 逐批讀取 PCM sample，避免一次把整首歌載進記憶體。
            while (file_stream->IsActive()) {
                const auto samples_read = file_stream->GetSamples(buffer.data(), kReadFrames);
                const auto frames_read = samples_read / channels;
                if (frames_read == 0) {
                    break;
                }

                // 將 interleaved channel sample 合併成每個 frame 的單一 peak。
                for (uint32_t frame = 0; frame < frames_read; ++frame) {
                    float peak = 0.0f;
                    const auto offset = frame * channels;
                    for (uint16_t channel = 0; channel < channels; ++channel) {
                        peak = (std::max)(peak, normalizedPeak(buffer[offset + channel]));
                    }

                    // 依照時間比例把 frame 放入對應的畫面 bucket，累積 RMS 需要的平方和。
                    const auto bucket = (std::min)(
                        target_peak_count - 1,
                        static_cast<int>((frame_index * target_peak_count) / total_frames));
                    bucket_square_sums[bucket] += static_cast<double>(peak) * peak;
                    ++bucket_counts[bucket];
                    ++frame_index;
                }
            }

            result.peaks.fill(0.0f, target_peak_count);
            // 將每個 bucket 的平方平均轉成 RMS，再用 soft-knee 壓成適合 UI 的高度。
            for (int i = 0; i < target_peak_count; ++i) {
                if (bucket_counts[i] == 0) {
                    continue;
                }
                const auto rms = std::sqrt(bucket_square_sums[i] / bucket_counts[i]);
                result.peaks[i] = static_cast<float>(1.0 - std::exp(-rms * kWaveformRmsGain));
            }
        }
        catch (const Exception& e) {
            result.error_message = QString::fromUtf8(e.GetErrorMessage());
        }
        catch (const std::exception& e) {
            result.error_message = QString::fromUtf8(e.what());
        }
        return result;
    }
}

WaveformSlider::WaveformSlider(QWidget* parent)
    : QSlider(Qt::Horizontal, parent) {
    min_ = minimum();
    max_ = maximum();
    animation_ = new QPropertyAnimation(this, "value", this);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_Hover, true);
    (void)QObject::connect(this, &QSlider::valueChanged, this, [this]() {
        update();
    });
}

void WaveformSlider::setRange(int64_t min, int64_t max) {
    min_ = min;
    max_ = max;
    QSlider::setRange(static_cast<int>(min), static_cast<int>(max));
    update();
}

void WaveformSlider::enableAnimation(bool enable) {
    if (!enable && animation_) {
        animation_->deleteLater();
        animation_ = nullptr;
    }
}

void WaveformSlider::setWaveformPeaks(const QVector<float>& peaks) {
    peaks_ = peaks;
    for (auto& peak : peaks_) {
        peak = normalizedPeak(peak);
    }
    update();
}

void WaveformSlider::clearWaveform() {
    cancelWaveformLoad();
    peaks_.clear();
    update();
}

void WaveformSlider::loadFile(const QString& file_path, int peak_count) {
    clearWaveform();
    if (file_path.isEmpty()) {
        return;
    }

    const auto load_id = waveform_load_id_;
    const auto target_peak_count = peak_count > 0 ? peak_count : (std::max)(kMinimumWidth, width());
    auto* watcher = new QFutureWatcher<WaveformReadResult>(this);
    current_load_watcher_ = watcher;

    (void)QObject::connect(watcher, &QFutureWatcher<WaveformReadResult>::finished, this, [this, watcher, file_path, load_id]() {
        watcher->deleteLater();
        if (load_id != waveform_load_id_) {
            return;
        }
        current_load_watcher_ = nullptr;

        const auto result = watcher->result();
        if (!result.error_message.isEmpty()) {
            emit waveformLoadFailed(file_path, result.error_message);
            return;
        }

        setWaveformPeaks(result.peaks);
        emit waveformLoadFinished(file_path);
    });

    emit waveformLoadStarted(file_path);
    watcher->setFuture(QtConcurrent::run([file_path, target_peak_count]() {
        return readWaveformPeaks(file_path, target_peak_count);
    }));
}

void WaveformSlider::setWaveformColors(const QColor& background,
    const QColor& foreground,
    const QColor& played) {
    background_color_ = background;
    foreground_color_ = foreground;
    played_color_ = played;
    update();
}

QSize WaveformSlider::sizeHint() const {
    return QSize(240, kDefaultHeight);
}

QSize WaveformSlider::minimumSizeHint() const {
    return QSize(kMinimumWidth, kDefaultHeight);
}

void WaveformSlider::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

    const auto rect = waveformRect();
    if (!rect.isValid()) {
        return;
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(background_color_);
    painter.drawRoundedRect(rect, kCornerRadius, kCornerRadius);

    drawWaveform(painter, rect, foreground_color_);

    const auto played_width = rect.width() * valueRatio();
    if (played_width > 0.0) {
        painter.save();
        painter.setClipRect(QRectF(rect.left(), rect.top(), played_width, rect.height()));
        drawWaveform(painter, rect, played_color_);
        painter.restore();
    }
}

void WaveformSlider::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        dragging_ = true;
        event->accept();

        const auto new_value = valueFromPosition(event->pos());
        setValueAnimation(static_cast<int>(new_value), true);
        emit leftButtonValueChanged(new_value);
        return;
    }
    QSlider::mousePressEvent(event);
}

void WaveformSlider::mouseMoveEvent(QMouseEvent* event) {
    if (dragging_ && (event->buttons() & Qt::LeftButton)) {
        event->accept();

        const auto new_value = valueFromPosition(event->pos());
        setValueAnimation(static_cast<int>(new_value), false);
        emit leftButtonValueChanged(new_value);
        return;
    }
    QSlider::mouseMoveEvent(event);
}

void WaveformSlider::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && dragging_) {
        dragging_ = false;
        event->accept();
        return;
    }
    QSlider::mouseReleaseEvent(event);
}

void WaveformSlider::sliderChange(SliderChange change) {
    QSlider::sliderChange(change);
    if (change == QAbstractSlider::SliderRangeChange) {
        min_ = minimum();
        max_ = maximum();
    }
    update();
}

QRectF WaveformSlider::waveformRect() const {
    return QRectF(kHorizontalPadding,
        kVerticalPadding,
        (std::max<qreal>)(0.0, width() - kHorizontalPadding * 2.0),
        (std::max<qreal>)(0.0, height() - kVerticalPadding * 2.0));
}

int64_t WaveformSlider::valueFromPosition(const QPoint& pos) const {
    if (max_ <= min_) {
        return min_;
    }

    const auto rect = waveformRect();
    double ratio = 0.0;
    if (orientation() == Qt::Horizontal) {
        ratio = rect.width() > 0.0 ? (pos.x() - rect.left()) / rect.width() : 0.0;
    }
    else {
        ratio = rect.height() > 0.0 ? (rect.bottom() - pos.y()) / rect.height() : 0.0;
    }

    ratio = std::clamp(ratio, 0.0, 1.0);
    const auto value = min_ + static_cast<int64_t>(std::llround((max_ - min_) * ratio));
    return std::clamp<int64_t>(value, min_, max_);
}

double WaveformSlider::valueRatio() const {
    if (max_ <= min_) {
        return 0.0;
    }
    return std::clamp((static_cast<double>(value()) - static_cast<double>(min_))
        / static_cast<double>(max_ - min_), 0.0, 1.0);
}

void WaveformSlider::setValueAnimation(int value, bool animate) {
    target_ = value;
    if (animate && animation_) {
        animation_->stop();
        animation_->setDuration(duration_);
        animation_->setEasingCurve(easing_curve_);
        animation_->setStartValue(QSlider::value());
        animation_->setEndValue(value);
        animation_->start();
        return;
    }
    QSlider::setValue(value);
    update();
}

void WaveformSlider::cancelWaveformLoad() {
    ++waveform_load_id_;
    if (current_load_watcher_) {
        current_load_watcher_->deleteLater();
        current_load_watcher_ = nullptr;
    }
}

void WaveformSlider::drawWaveform(QPainter& painter, const QRectF& rect, const QColor& color) {
    painter.save();
    painter.setPen(QPen(color, kWaveformBarWidth, Qt::SolidLine, Qt::RoundCap));

    const auto bar_pitch = kWaveformBarWidth + kWaveformBarGap;
    const int bar_count = (std::max)(1, static_cast<int>(std::floor((rect.width() + kWaveformBarGap) / bar_pitch)));
    const auto center_y = rect.center().y();
    const auto max_half_height = (std::max<qreal>)(0.0, (rect.height() - kWaveformBarWidth) * 0.5);
    const auto waveform_width = (bar_count - 1) * bar_pitch + kWaveformBarWidth;
    const auto start_x = rect.left() + (rect.width() - waveform_width) * 0.5 + kWaveformBarWidth * 0.5;

    for (int i = 0; i < bar_count; ++i) {
        float peak = 0.0f;
        if (!peaks_.isEmpty()) {
            const int begin = i * peaks_.size() / bar_count;
            const int end = (std::max)(begin + 1, (i + 1) * static_cast<int>(peaks_.size()) / bar_count);
            for (int j = begin; j < end && j < peaks_.size(); ++j) {
                peak = (std::max)(peak, peaks_.at(j));
            }
        }
        else {
            const auto phase = static_cast<double>(i) / static_cast<double>((std::max)(1, bar_count - 1));
            peak = static_cast<float>(0.12 + 0.08 * std::sin(phase * XAMP_PI * 6.0));
        }

        const auto half_height = (std::max<qreal>)(kMinimumBarHeight * 0.5, max_half_height * peak);
        const auto x = start_x + i * bar_pitch;
        painter.drawLine(QPointF(x, center_y - half_height), QPointF(x, center_y + half_height));
    }

    painter.restore();
}
