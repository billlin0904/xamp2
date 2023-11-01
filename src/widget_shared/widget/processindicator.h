//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QWidget>
#include <QColor>

class ProcessIndicator : public QWidget {
    Q_OBJECT
    Q_PROPERTY(int delay READ AnimationDelay WRITE SetAnimationDelay)
    Q_PROPERTY(bool displayedWhenStopped READ IsDisplayedWhenStopped WRITE SetDisplayedWhenStopped)
    Q_PROPERTY(QColor color READ color WRITE SetColor)

public:
    explicit ProcessIndicator(QWidget* parent = nullptr);

    int AnimationDelay() const { return delay_; }

    bool IsAnimated() const;

    bool IsDisplayedWhenStopped() const;

    const QColor& color() const { return color_; }

    QSize sizeHint() const override;

    int heightForWidth(int w) const override;

    void SetStoppedIcon(const QIcon &icon);

public slots:
    void StartAnimation();

    void StopAnimation();

    void SetAnimationDelay(int delay);

    void SetDisplayedWhenStopped(bool state);

    void SetColor(const QColor& color);

protected:
    void timerEvent(QTimerEvent* event) override;

    void paintEvent(QPaintEvent* event) override;

private:
    int angle_;
    int timer_id_;
    int delay_;
    bool displayed_when_stopped_;
    QColor color_;
    QIcon stopped_icon_;
};
