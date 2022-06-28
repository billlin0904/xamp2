//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QWidget>
#include <QColor>

class ProcessIndicator : public QWidget {
    Q_OBJECT
    Q_PROPERTY(int delay READ animationDelay WRITE setAnimationDelay)
    Q_PROPERTY(bool displayedWhenStopped READ isDisplayedWhenStopped WRITE setDisplayedWhenStopped)
    Q_PROPERTY(QColor color READ color WRITE setColor)

public:
    explicit ProcessIndicator(QWidget* parent = nullptr);

    int animationDelay() const { return delay_; }

    bool isAnimated() const;

    bool isDisplayedWhenStopped() const;

    const QColor& color() const { return color_; }

    virtual QSize sizeHint() const;

    int heightForWidth(int w) const;

public slots:
    void startAnimation();

    void stopAnimation();

    void setAnimationDelay(int delay);

    void setDisplayedWhenStopped(bool state);

    void setColor(const QColor& color);

protected:
    void timerEvent(QTimerEvent* event) override;

    void paintEvent(QPaintEvent* event) override;

private:
    int angle_;
    int timer_id_;
    int delay_;
    bool displayed_when_stopped_;
    QColor color_;
};
