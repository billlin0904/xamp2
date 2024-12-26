//=====================================================================================================================
// Copyright (c) 2018-2025 XAMP project. All rights reserved.
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

class XAMP_WIDGET_SHARED_EXPORT XTooltip : public QWidget {
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

    void setTextAlignment(Qt::Alignment alignment);

	void setTextFont(const QFont& font);

    void setText(const QString& text);

    QString text() const;

    void showAndStart(bool start_timer = true);

    void setImageSize(const QSize& size);

    void setImage(const QPixmap& pixmap);

    void onThemeChangedFinished(ThemeColor theme_color);
protected:
    bool eventFilter(QObject* obj, QEvent* e) override;

    void paintEvent(QPaintEvent* event) override;

private:
    QLabel* text_;
    QLabel* image_;
	QTimer timer_;
	int32_t max_width_ = 0;
};
