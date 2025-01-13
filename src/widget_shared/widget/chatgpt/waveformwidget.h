//=====================================================================================================================
// Copyright (c) 2018-2025 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFrame>
#include <QWidget>
#include <QPainterPath>

#include <widget/widget_shared_global.h>
#include <widget/widget_shared.h>
#include <vector>

constexpr uint32_t DRAW_ONLY_RIGHT_CHANNEL = 1;
constexpr uint32_t DRAW_ONLY_LEFT_CHANNEL  = 1 << 2;
constexpr uint32_t DRAW_BOTH_CHANNEL       = 1 << 3;
constexpr uint32_t DRAW_PLAYED_AREA        = 1 << 4;
constexpr uint32_t DRAW_SPECTROGRAM        = 1 << 5;

class XAMP_WIDGET_SHARED_EXPORT WaveformWidget : public QFrame {
    Q_OBJECT

public:
    static constexpr auto kLeftPlayedChannelColor = QColor(50, 255, 50, 180);
    static constexpr auto kLeftUnPlayedChannelColor = QColor(128, 128, 128, 180);
    static constexpr auto kRightPlayedChannelColor = QColor(50, 255, 50, 180);
    static constexpr auto kRightUnPlayedChannelColor = QColor(0, 192, 192, 180);

    static constexpr float kHeadroomFactor = 0.6f;
    static constexpr float kYTextHeight = 20.0f;
    static constexpr float kCornerRadius = 5.0f;
    static constexpr uint32_t kPadding = 4;
    static constexpr uint32_t kFramesPerPeak = 4096;
    static constexpr int kTextOffsetY = -10;

    explicit WaveformWidget(QWidget *parent = nullptr);

    void setCurrentPosition(float sec);

    void setTotalDuration(float duration);

    void setSampleRate(uint32_t sample_rate);

    void setFramePerPeekSize(size_t size);

    void setDrawMode(uint32_t mode);

signals:
    void playAt(float sec);

public slots:
    void onReadAudioData(const std::vector<float> & buffer);

    void setSpectrogramData(const QImage& spectrogramImg);

    void doneRead();

    void clear();

protected:
    void updateSpectrogramSize();

    void paintEvent(QPaintEvent *event) override;

    void mousePressEvent(QMouseEvent* event) override;

	void mouseMoveEvent(QMouseEvent* event) override;

    void mouseReleaseEvent(QMouseEvent* event) override;

    void resizeEvent(QResizeEvent* event) override;

private:
    void updateCachePixmap();

    QPainterPath buildChannelPath(const std::vector<float>& peaks, int startIndex, int endIndex, int top, int channelH) const;

    void updatePlayedPaths(int playIndex);

    void drawTimeAxis(QPainter& p);

    void drawDuration(QPainter& painter);

    float mapPeakToY(float peakVal, int top, int height, bool isPositive) const;

    float xToTime(float x) const;

    float timeToX(float sec) const;

    uint32_t draw_mode_ = DRAW_ONLY_RIGHT_CHANNEL;
    float total_ms_ = 0.f;
    size_t peak_count_ = 0;
    float cursor_ms_ = -1.f;
    uint32_t sample_rate_ = 44100;
	size_t frame_per_peak_ = kFramesPerPeak;
    std::vector<float> left_peaks_;
    std::vector<float> right_peaks_;
    QPixmap cache_;
    QPixmap spectrogram_;
    QPixmap spectrogram_cache_;
    QPainterPath path_left_played_;
    QPainterPath path_right_played_;
};
