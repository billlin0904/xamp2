//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once


#include <QApplication>
#include <QDialog>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QTimer>
#include <QMouseEvent>
#include <QGraphicsDropShadowEffect>

#include <widget/widget_shared_global.h>
#include <widget/xdialog.h>
#include <widget/util/str_util.h>

class XAMP_WIDGET_SHARED_EXPORT XTooltip : public XDialog {
    Q_OBJECT

public:
    explicit XTooltip(const QString& text = QString(), QWidget* parent = nullptr);

    static void popup(QPoint pos, const QString& text) {
        auto* t = new XTooltip(text);
        t->show();
        t->move(pos);
    }

    static void popup(const QString& text) {
        XTooltip::popup(QCursor::pos(), text);
    }

    void setText(const QString& text);

    QString text() const;

    void showAndStart();

    void onThemeChangedFinished(ThemeColor theme_color);
protected:
    bool eventFilter(QObject* obj, QEvent* e);

private:
    QLabel* text_;
	QTimer timer_;
	int32_t max_width_ = 0;
};