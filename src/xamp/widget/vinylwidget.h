//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QWidget>
#include <QPixmap>
#include <QTimer>

class QPaintEvent;

class VinylWidget : public QWidget {
    Q_OBJECT
public:
    explicit VinylWidget(QWidget *parent = nullptr);

public slots:
    void start();

    void stop();

    void setPixmap(QPixmap const &image);

private:
    void writeBackground();

    void paintEvent(QPaintEvent *) override;

    double angle_;
    QTimer timer_;
    QPixmap background_;
    QPixmap cover_;
    QPixmap vinly_;
    QPixmap image_;
};

