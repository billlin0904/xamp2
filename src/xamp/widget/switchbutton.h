#pragma once

#include <QPushButton>

class SwitchButton : public QPushButton {
    Q_OBJECT
public:
    explicit SwitchButton(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *ev) override;

    void nextCheckState() override;
private:
    bool checked_{false};
    qreal progress_{0};
};
