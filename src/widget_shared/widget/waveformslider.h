//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QColor>
#include <QEasingCurve>
#include <QPoint>
#include <QPointer>
#include <QRectF>
#include <QSize>
#include <QSlider>
#include <QString>
#include <QVariantAnimation>
#include <QVector>

#include <widget/widget_shared_global.h>

#include <cstdint>

class QMouseEvent;
class QPainter;
class QPaintEvent;

class XAMP_WIDGET_SHARED_EXPORT WaveformSlider final : public QSlider {
    Q_OBJECT

public:
    explicit WaveformSlider(QWidget* parent = nullptr);

    void setRange(int64_t min, int64_t max);

    void enableAnimation(bool enable);

    void setWaveformPeaks(const QVector<float>& peaks);

    void clearWaveform();

    void loadFile(const QString& file_path, int peak_count = 0);

    void setWaveformColors(const QColor& background,
        const QColor& foreground,
        const QColor& played);

    QSize sizeHint() const override;

    QSize minimumSizeHint() const override;

signals:
    void leftButtonValueChanged(int64_t value);

    void waveformLoadStarted(const QString& file_path);

    void waveformLoadFinished(const QString& file_path);

    void waveformLoadFailed(const QString& file_path, const QString& message);

protected:
    void paintEvent(QPaintEvent* event) override;

    void mousePressEvent(QMouseEvent* event) override;

    void mouseMoveEvent(QMouseEvent* event) override;

    void mouseReleaseEvent(QMouseEvent* event) override;

    void sliderChange(SliderChange change) override;

private:
    QRectF waveformRect() const;

    int64_t valueFromPosition(const QPoint& pos) const;

    double valueRatio() const;

    void setValueAnimation(int value, bool animate);

    void cancelWaveformLoad();

    void drawWaveform(QPainter& painter, const QRectF& rect, const QColor& color);

    int target_ = 0;
    int duration_ = 100;
    int64_t min_ = 0;
    int64_t max_ = 0;
    QPointer<QVariantAnimation> animation_;
    QPointer<QObject> current_load_watcher_;
    QEasingCurve easing_curve_ = QEasingCurve(QEasingCurve::Type::InOutCirc);
    QVector<float> peaks_;
    QColor background_color_ = QColor(30, 34, 42);
    QColor foreground_color_ = QColor(94, 103, 120);
    QColor played_color_ = QColor(64, 169, 255);
    bool dragging_ = false;
    int waveform_load_id_ = 0;
};
