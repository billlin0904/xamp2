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
    static constexpr uint32_t kDrawOnlyRightChannel = 1 << 0;
    static constexpr uint32_t kDrawOnlyLeftChannel = 1 << 2;
    static constexpr uint32_t kDrawBothChannel = 1 << 3;
    static constexpr uint32_t kDrawPlayedArea = 1 << 4;
    static constexpr uint32_t kDrawSpectrogram = 1 << 5;
    static constexpr uint32_t kDrawOnlyRightChRms = 1 << 6;
    static constexpr uint32_t kDrawOnlyLeftChRms = 1 << 7;
    static constexpr uint32_t kDrawBothChannelRms = 1 << 8;

    //static constexpr auto kLeftPlayedChannelColor = QColor(50, 255, 50, 180);
    //static constexpr auto kLeftUnPlayedChannelColor = QColor(33, 150, 243);
    //static constexpr auto kRightPlayedChannelColor = QColor(50, 255, 50, 180);
    //static constexpr auto kRightUnPlayedChannelColor = QColor(33, 150, 243);

    static constexpr auto kRmsColor = QColor(255, 255, 255, 120);

    //static constexpr uint32_t kFramesPerPeak = 4096;
    static constexpr float kHeadroomFactor = 0.6f;
    static constexpr float kYTextHeight = 20.0f;
    static constexpr float kCornerRadius = 5.0f;
    static constexpr uint32_t kPadding = 4;    
    static constexpr int kTextOffsetY = -10;

    explicit WaveformWidget(QWidget *parent = nullptr);

    void setCurrentPosition(float sec);

    void setTotalDuration(float duration);

    void setSampleRate(uint32_t sample_rate);

    void setDrawMode(uint32_t mode);

signals:
    void playAt(float sec);

    void readAudioSpectrogram(const QSize& widget_size, const Path& file_path);

public slots:
    void setSpectrogramData(double duration_sec, const QImage& chunk, int time_index);

    void doneRead();

    void clear();

    void loadFile(const Path& file_path);

protected:
    void updateSpectrogramSize();

    void paintEvent(QPaintEvent *event) override;

    void mousePressEvent(QMouseEvent* event) override;

	void mouseMoveEvent(QMouseEvent* event) override;

    void mouseReleaseEvent(QMouseEvent* event) override;

    void resizeEvent(QResizeEvent* event) override;

private:
    void drawTimeAxis(QPainter& painter, const QRect& rect);

    void drawDuration(QPainter& painter, const QRect& rect);

    void drawFrequencyAxis(QPainter& painter, const QRect& rect);

    float mapPeakToY(float peakVal, int top, int height, bool isPositive) const;

    float xToTime(float x, const QRect& rect) const;

    float timeToX(float sec, const QRect& rect) const;

    float mapFreqToY(float freq, const QRect& rect) const;

    void markCacheDirty();

    void updateStaticCache();

    void drawGainColorBar(QPainter& painter, const QRect& barRect);

    void drawGainColorBarTicks(QPainter& painter, const QRect& barRect);

    QRect gainColorBarRect() const;

    QRect drawRect() const;

    bool cache_dirty_ = true;
    bool is_processing_ = false;
    uint32_t draw_mode_ = kDrawSpectrogram;
    float total_ms_ = 0.f;
    float cursor_ms_ = -1.f;
	uint32_t sample_rate_ = 44100;
	Path file_path_;
    QImage spectrogram_;
    QImage spectrogram_cache_;
    QPixmap static_cache_;
};
