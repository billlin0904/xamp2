//=====================================================================================================================
// Copyright (c) 2018-2025 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QMap>
#include <QListView>
#include <QStandardItemModel>
#include <QElapsedTimer>
#include <QVBoxLayout>
#include <QPainter>

#include <widget/fonticon.h>
#include <widget/themecolor.h>
#include <widget/widget_shared_global.h>
#include <widget/util/str_util.h>

#include "thememanager.h"

enum TabIndex {
    TAB_PLAYLIST,
    TAB_FILE_EXPLORER,
    TAB_LYRICS,
    TAB_MUSIC_LIBRARY,
    TAB_CD,
    TAB_YT_MUSIC_SEARCH,
    TAB_YT_MUSIC_PLAYLIST,
    TAB_AI,
};

class XTooltip;
class FontIcon;
class QWheelEvent;

class NavWidget : public QWidget {
    Q_OBJECT
public:
    static constexpr int kExpandWidth = 180;

    explicit NavWidget(bool selectable, QWidget* parent = nullptr)
        : QWidget(parent) {
        isSelectable = selectable;
        setFixedSize(40, 36);
    }

    virtual void setIcon(const QIcon& icon) {	    
    }

    virtual void setCompacted(bool compacted) {
        if (isCompacted == compacted) {
            return;
        }

        isCompacted = compacted;
        if (isCompacted) {
            setFixedSize(40, 36);
        }
        else {
            setFixedSize(kExpandWidth, 36);
        }

        update();
    }

    void setSelected(bool selected) {
        if (!isSelectable || (isSelected == selected)) {
            return;
        }

        isSelected = selected;
        update();
    }

    bool isCompacted{ true };
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

class NavSeparator : public NavWidget {
    Q_OBJECT
public:
    explicit NavSeparator(QWidget* parent = nullptr)
        : NavWidget(parent) {
        NavSeparator::setCompacted(true);
    }

    void setCompacted(bool compacted) override {
        if (compacted) {
            setFixedSize(48, 3);
        }
        else {
            setFixedSize(kExpandWidth + 10, 3);
        }

        update();
    }

    void paintEvent(QPaintEvent* /*event*/) {
        QPainter painter(this);
        int c = qTheme.isDarkTheme() ? 255 : 0;
        QPen pen(QColor(c, c, c, 15));
        pen.setCosmetic(true);
        painter.setPen(pen);
        painter.drawLine(0, 1, width(), 1);
    }
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
        else {
        	//painter.setOpacity(0.4);
        }

        int c = qTheme.isDarkTheme() ? 255 : 0;
        if (isSelected) {
            if (isEnter) {
                painter.setBrush(QColor(c, c, c, 6));
            }
            else {
                painter.setBrush(QColor(c, c, c, 10));
            }
            painter.drawRoundedRect(rect(), 5, 5);

            painter.setBrush(qTheme.indicatorColor());
            painter.drawRoundedRect(0, 10, 3, 16, 1.5, 1.5);
        }
        else {
            painter.setBrush(QColor(c, c, c, 10));
            painter.drawRoundedRect(rect(), 5, 5);
        }

        auto pixmap = icon_.pixmap(QSize(16, 16));
        painter.drawPixmap(QRectF(11.5, 10, 16, 16).toRect(), pixmap);

        if (!isCompacted) {
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

class NavToolButton : public NavPushButton {
	Q_OBJECT
public:
	explicit NavToolButton(const QIcon& glyphs, QWidget* parent = nullptr)
        : NavPushButton(glyphs, kEmptyString, false, parent) {
    }

    void setCompacted(bool /*compacted*/) {
        setFixedSize(40, 36);
    }
};


class NavItemLayout : public QVBoxLayout {
    Q_OBJECT
public:
    explicit NavItemLayout(QWidget* parent = nullptr)
		: QVBoxLayout(parent) {
    }

    void setGeometry(const QRect& rect) override {
        QVBoxLayout::setGeometry(rect);
        for (int i = 0; i < this->count(); ++i) {
            QLayoutItem* item = this->itemAt(i);
            auto* ns = dynamic_cast<NavSeparator*>(item->widget());
            if (ns) {
                QRect geo = item->geometry();
                item->widget()->setGeometry(0, geo.y(), geo.width(), geo.height());
            }
        }
    }
};

class QScrollArea;

class XAMP_WIDGET_SHARED_EXPORT NavBarListView final : public QFrame /*QListView*/ {
    Q_OBJECT
public:
    explicit NavBarListView(QWidget *parent = nullptr);

    void addTab(const QString& name, int table_id, const QIcon& icon);

    void addSeparator();

    QString tabName(int table_id) const;

    int32_t tabId(const QString &name) const;

	int32_t currentTabId() const;

    void setTabText(const QString& name, int table_id);

    void mouseMoveEvent(QMouseEvent* event) override;

    void wheelEvent(QWheelEvent* event) override;

    void toolTipMove(const QPoint &pos);

    void setCurrentIndex(int32_t tab_id);

    void collapse();

    void expand();

signals:
    void clickedTable(int table_id);

    void tableNameChanged(int table_id, const QString &name);

public slots:
    void onThemeChangedFinished(ThemeColor theme_color);
    
    void onRetranslateUi();

private:
    QHash<int32_t, QString> names_;
    QHash<QString, int> ids_;
	QHash<int32_t, NavWidget*> widgets_;
    XTooltip* tooltip_;
	QElapsedTimer elapsed_timer_;

    QWidget* scroll_widget_;
	QScrollArea* scroll_area_;
    NavToolButton* return_btn_;
    NavToolButton* menu_btn_;
    NavItemLayout* main_layout_;
    NavItemLayout* top_layout_;
    NavItemLayout* scroll_layout_;
	NavItemLayout* bottom_layout_;
};

