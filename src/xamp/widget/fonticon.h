//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>
#include <QPainter>
#include <QIconEngine>
#include <QApplication>
#include <QtCore>
#include <QPalette>

#include <widget/widget_shared.h>

class FontIconEngine : public QIconEngine {
public:
    FontIconEngine();

    void paint(QPainter* painter, const QRect& rect, QIcon::Mode mode, QIcon::State state) override;

    QPixmap pixmap(const QSize& size, QIcon::Mode mode, QIcon::State state) override;

    QIconEngine* clone() const override;

    void setFontFamily(const QString& family);

    void setLetter(const QChar& letter);

    void setBaseColor(const QColor& base_color);

    void setSelectedState(bool enable);
private:
    bool selected_state_;
    QString font_family_;
    QChar letter_;
    QColor base_color_;
};

class FontIcon : public QObject {
public:
    explicit FontIcon(QObject* parent = nullptr);

    bool addFont(const QString& filename);

    QIcon icon(const QChar& code, const QColor *color = nullptr, const QString& family = QString()) const;

    const QStringList& families() const;

    void setBaseColor(const QColor& base_color);

    QColor baseColor() const {
        return base_color_;
    }

protected:
    void addFamily(const QString& family);

    QColor base_color_;
    QStringList families_;
};

#define qFontIcon SharedSingleton<FontIcon>::GetInstance()
#define Q_FONT_ICON_CODE_COLOR(code, color) qFontIcon.icon(code, color)
#define Q_FONT_ICON(code) qFontIcon.icon(code)