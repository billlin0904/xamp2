//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QVBoxLayout>
#include <QPainter>

#include <widget/fonticon.h>
#include <widget/themecolor.h>
#include <widget/widget_shared_global.h>
#include <widget/util/str_util.h>

#include "thememanager.h"

enum TabIndex {
    TAB_LYRICS,
    TAB_RICH_PLAYLIST,
    TAB_FILE_EXPLORER,
    TAB_CD = 4
};

class FontIcon;
class QWheelEvent;

class NavWidget : public QWidget {
    Q_OBJECT
public:
    static constexpr int kExpandWidth = 172;

    explicit NavWidget(bool selectable, QWidget* parent = nullptr)
        : QWidget(parent) {
        isSelectable = selectable;
        setFixedSize(kExpandWidth, 44);
    }

    virtual void setIcon(const QIcon& icon) {	    
    }

    void setSelected(bool selected) {
        if (!isSelectable || (isSelected == selected)) {
            return;
        }

        isSelected = selected;
        update();
    }

    bool isSelected{ false };
    bool isPressed{ false };
    bool isEnter{ false };
    bool isSelectable{ false };

protected:
    void mousePressEvent(QMouseEvent* event) override {
        isPressed = true;
        update();
    }

    void mouseReleaseEvent(QMouseEvent* event) override {
        isPressed = false;
        update();
        emit clicked(true);
    }

    void enterEvent(QEnterEvent* event) override {
        isEnter = true;
        update();
    }

    void leaveEvent(QEvent* event) override {
        isEnter = false;
        isPressed = false;
        update();
    }
signals:
    void clicked(bool);
};

class NavPushButton : public NavWidget {
    Q_OBJECT
public:
    explicit NavPushButton(const QIcon& icon,
        const QString& text,
        bool selectable,
        QWidget* parent = nullptr)
        : NavWidget(selectable, parent) {
        icon_ = icon;
        text_ = text;
        isSelectable = selectable;
    }

    void setIcon(const QIcon& icon) override {
        icon_ = icon;
		update();
    }

    void setText(const QString& text) { text_ = text; update(); }

    QString text() const {
        return text_;
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        QPainter painter(this);
        painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
        painter.setPen(Qt::NoPen);

        if (isPressed) {
            painter.setOpacity(0.7);
        }

        if (isSelected || isEnter) {
            painter.setBrush(isSelected ? qTheme.highlightColor() : qTheme.hoverColor());
            painter.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 8, 8);
        }
        auto pixmap = icon_.pixmap(QSize(18, 18));
        painter.drawPixmap(QRect(11, (height() - 18) / 2, 18, 18), pixmap);

        {
            painter.setFont(font());
            
            if (!qTheme.isDarkTheme()) {
				painter.setPen(Qt::black);
            } else {
                painter.setPen(Qt::white);
            }
            painter.drawText(QRect(44, 0, width() - 57, height()), Qt::AlignVCenter, text_);
        }
    }

    QIcon icon_;
    QString text_;
};

class QScrollArea;

class XAMP_WIDGET_SHARED_API NavBarListView final : public QFrame {
    Q_OBJECT
public:
    void setTabText(int id, const QString& text);
    explicit NavBarListView(QWidget* parent = nullptr);
    void addTab(const QString& name, int tab_id, const QIcon& icon);
    void setCurrentIndex(int32_t tab_id);

signals:
    void clickedTable(int tab_id);

private slots:
    void onThemeChangedFinished(ThemeColor theme_color);

private:
    QHash<int32_t, NavWidget*> widgets_;
    QVBoxLayout* scroll_layout_;
};
