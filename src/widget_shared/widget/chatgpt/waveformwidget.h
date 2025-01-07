//=====================================================================================================================
// Copyright (c) 2018-2025 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFrame>
#include <QWidget>
#include <widget/widget_shared_global.h>
#include <widget/widget_shared.h>
#include <vector>

class XAMP_WIDGET_SHARED_EXPORT WaveformWidget : public QFrame {
    Q_OBJECT

public:
    static constexpr int kFramesPerPeak = 4096;

    explicit WaveformWidget(QWidget *parent = nullptr);

    void setCurrentPosition(float sec);

    void setSampleRate(uint32_t sample_rate);

signals:
    void playAt(float sec);

public slots:
    void onReadAudioData(const std::vector<float> & buffer);

    void updateWavePixmap();

    void doneRead();

protected:
    void paintEvent(QPaintEvent *event) override;

    void mousePressEvent(QMouseEvent* event) override;

	void mouseMoveEvent(QMouseEvent* event) override;

    void mouseReleaseEvent(QMouseEvent* event) override;

    void resizeEvent(QResizeEvent* event) override;

private:
    void drawTimeAxis(QPainter& p);

    void drawDuration(QPainter& painter);

    float mapPeakToY(float peakVal, int top, int height, bool isPositive) const;

    float xToTime(float x) const;

    float timeToX(float sec) const;

    bool pixmap_dirty_ = true;
    float total_ms_ = 0.f;
    size_t  peak_count_ = 0;
    float cursor_ms_ = -1.f;
    uint32_t sample_rate_ = 44100;
    QPixmap wave_cache_;
    QPixmap mask_pixmap_;
    std::vector<float> left_peaks_;
    std::vector<float> right_peaks_;
};
