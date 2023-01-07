#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>

#include <widget/win32/win32.h>

#include <widget/image_utiltis.h>
#include <widget/str_utilts.h>
#include <widget/tooltips.h>

#include "thememanager.h"

ToolTips::ToolTips(const QString& text, QWidget* parent)
    : QFrame(parent) {
	setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground, true);
    
    setObjectName(qTEXT("ToolTips"));
    setStyleSheet(qTEXT("QFrame#ToolTips { background-color: transparent }"));
#ifdef XAMP_OS_WIN
    win32::addDwmMenuShadow(winId());
#endif
    setContentsMargins(0, 0, 0, 0);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(7, 4, 7, 4);
    layout->setSpacing(0);

	auto* inter_frame = new QFrame(this);
    inter_frame->setContentsMargins(0, 0, 0, 0);
    auto* interlayout = new QHBoxLayout(inter_frame);
    interlayout->setContentsMargins(1, 1, 1, 1);
    interlayout->setSpacing(5);

    text_label_ = new QLabel(text);
    text_label_->setObjectName(qTEXT("TipText"));
    text_label_->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    text_label_->setWordWrap(false);
    interlayout->addWidget(text_label_, 0, Qt::AlignVCenter);
    layout->addWidget(inter_frame, 0, Qt::AlignVCenter);

    adjustSize();

    qTheme.setBackgroundColor(this);

    hide();
}

void ToolTips::enterEvent(QEvent*) {
    hide();
}

void ToolTips::setText(const QString& text) {
    text_label_->setText(text);
    update();
}

void ToolTips::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.setPen(pen_);
    painter.setBrush(brush_);

    QRect rect = this->rect();
    rect.setWidth(rect.width());
    rect.setHeight(rect.height());
    painter.fillRect(rect, qTheme.backgroundColor());

    /*QPolygon triangle;
    triangle << QPoint(0, -40) << QPoint(25, 40) << QPoint(-25, 40);
    painter.drawPolygon(triangle);*/

    QPainterPath painter_path;
    painter_path.addRoundedRect(rect, ImageUtils::kImageRadius, ImageUtils::kImageRadius);
    painter.drawPath(painter_path);
}
