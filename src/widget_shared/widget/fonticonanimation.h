//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>
#include <QTransform>

class QPainter;
class QTimer;
class QWidget;

class FontIconAnimation : public QObject {
    Q_OBJECT
public:
    explicit FontIconAnimation(QWidget* parent = nullptr);

    void paint(QPainter& painter, const QRect& rect);

    virtual void transform(QPainter& painter, QSize size) = 0;

public slots:
    void frame();

protected:
    int min_frame_{ 0 };
    int max_frame_{ 0 };
    int frame_{ 0 };

private:
    QTimer* timer_;
};

class FontIconSpinAnimation : public FontIconAnimation {
public:
    enum Directions {
        CLOCKWISE,
        ANTI_CLOCKWISE
    };

    explicit FontIconSpinAnimation(QWidget* parent = nullptr, Directions direction = Directions::CLOCKWISE, int rpm = 60);

    void transform(QPainter& painter, QSize size) override;
private:
    Directions direction_;
};

class PixmapGenerator : public QObject {
public:
    explicit PixmapGenerator(FontIconAnimation* animation, QWidget* parent = nullptr);

    QPixmap pixmap(QSize size);

private:
    FontIconAnimation* animation_;
    QMap<int, QPixmap> pixmap_cache_;
};

Q_DECLARE_METATYPE(FontIconSpinAnimation*)