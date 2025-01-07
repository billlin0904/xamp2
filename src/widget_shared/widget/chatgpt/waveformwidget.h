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

    explicit WaveformWidget(QWidget *parent = nullptr);

    void setCurrentPosition(float sec);

    void setSampleRate(uint32_t sample_rate);

signals:
    void playAt(float sec);

public slots:
    void onReadAudioData(const std::vector<float> & buffer);

    void doneRead();

protected:
    void paintEvent(QPaintEvent *event) override;

    void mousePressEvent(QMouseEvent* event) override;

	void mouseMoveEvent(QMouseEvent* event) override;

    void mouseReleaseEvent(QMouseEvent* event) override;

    void resizeEvent(QResizeEvent* event) override;

private:
    void updateUnplayedPixmap();

    QPainterPath buildChannelPath(const std::vector<float>& peaks, int startIndex, int endIndex, int top, int channelH) const;

    void updatePlayedPaths(int playIndex);

    void drawTimeAxis(QPainter& p);

    void drawDuration(QPainter& painter);

    float mapPeakToY(float peakVal, int top, int height, bool isPositive) const;

    float xToTime(float x) const;

    float timeToX(float sec) const;

    float total_ms_ = 0.f;
    size_t  peak_count_ = 0;
    float cursor_ms_ = -1.f;
    uint32_t sample_rate_ = 44100;
    std::vector<float> left_peaks_;
    std::vector<float> right_peaks_;
    QPixmap unplayed_cache_;
    QPainterPath path_left_played_;
    QPainterPath path_right_played_;
};
