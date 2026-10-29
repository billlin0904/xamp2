#pragma once

#include <QElapsedTimer>
#include <QPixmap>
#include <QTimer>
#include <QWidget>

#include <widget/widget_shared_global.h>

// A painted turntable: album art rotates with the record; the tonearm is independent.
class XAMP_WIDGET_SHARED_API VinylWidget final : public QWidget {
public:
    explicit VinylWidget(QWidget* parent = nullptr);

    void setCover(const QPixmap& cover);
    void setPlaying(bool playing);
    void setProgress(double progress);
    void setDarkTheme(bool dark);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private:
    void updateAnimation();
    void advanceAnimation();
    void rebuildRecord();

    QPixmap cover_;
    QPixmap record_;
    QTimer timer_;
    QElapsedTimer frame_clock_;
    qreal angle_{0.0};
    qreal arm_position_{0.0};
    double progress_{0.0};
    double arm_progress_{0.0};
    bool playing_{false};
    bool dark_{true};
};
