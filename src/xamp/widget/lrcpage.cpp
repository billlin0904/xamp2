#include <QLabel>
#include <QHBoxLayout>
#include <QPainter>
#include <QGraphicsDropShadowEffect>

#include "thememanager.h"

#include <widget/image_utiltis.h>
#include <widget/vinylwidget.h>
#include <widget/scrolllabel.h>
#include <widget/lyricsshowwideget.h>
#include <widget/str_utilts.h>
#include <widget/lrcpage.h>

LrcPage::LrcPage(QWidget* parent)
	: QFrame(parent) {
	initial();
}

LyricsShowWideget* LrcPage::lyricsWidget() {
	return lyrics_widget_;
}

QLabel* LrcPage::cover() {
    return cover_label_;
}

ScrollLabel* LrcPage::album() {
	return album_;
}

ScrollLabel* LrcPage::artist() {
	return artist_;
}

ScrollLabel* LrcPage::title() {
	return title_;
}

void LrcPage::paintEvent(QPaintEvent*) {
    QPainter painter(this);
	painter.drawPixmap(rect(), background_image_);
}

void LrcPage::setBackground(const QPixmap& cover) {
	if (cover.isNull()) {
		background_image_ = QPixmap();
	}
	else {
		background_image_ = Pixmap::blurImage(cover, 40);
	}
	update();
}

void LrcPage::resizeEvent(QResizeEvent*) {
}

void LrcPage::OnThemeColorChanged(QColor theme_color, QColor color) {
	lyrics_widget_->setLrcColor(color);
	lyrics_widget_->setLrcHightLight(color);
}

void LrcPage::setEffect(QWidget* widget, int blurRadius) {
	auto effect = new QGraphicsDropShadowEffect();
	effect->setOffset(0, 0);
	effect->setColor(Qt::black);
	effect->setBlurRadius(blurRadius);
	widget->setGraphicsEffect(effect);
}

void LrcPage::initial() {
	auto horizontalLayout_10 = new QHBoxLayout(this);
	
	horizontalLayout_10->setSpacing(0);
	horizontalLayout_10->setObjectName(QString::fromUtf8("horizontalLayout_10"));
	horizontalLayout_10->setContentsMargins(80, 80, 80, 80);
	horizontalLayout_10->setStretch(1, 1);

	auto verticalLayout_3 = new QVBoxLayout();
	verticalLayout_3->setSpacing(0);
	verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));	

    cover_label_ = new QLabel(this);
    cover_label_->setObjectName(QString::fromUtf8("label"));
    cover_label_->setMinimumSize(QSize(250, 250));
    cover_label_->setMaximumSize(QSize(250, 250));
	setEffect(cover_label_, 25);

    verticalLayout_3->addWidget(cover_label_);
	verticalLayout_3->setContentsMargins(0, 20, 0, 0);

	auto verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

	verticalLayout_3->addItem(verticalSpacer);

	horizontalLayout_10->addLayout(verticalLayout_3);

	auto horizontalSpacer_4 = new QSpacerItem(50, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

	horizontalLayout_10->addItem(horizontalSpacer_4);

	auto verticalLayout_2 = new QVBoxLayout();

	verticalLayout_2->setSpacing(0);
	verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));

	auto f = font();
    f.setPointSize(12);
	f.setBold(true);

    title_ = new ScrollLabel(this);
	title_->setStyleSheet(Q_UTF8("background-color: transparent"));
	title_->setObjectName(QString::fromUtf8("label_2"));
	title_->setText(tr("Title:"));	
	title_->setFont(f);
#ifdef Q_OS_WIN
    title_->setMinimumHeight(40);
    f.setPointSize(12);
#else
    title_->setMinimumHeight(50);
    f.setPointSize(20);
#endif
    f.setBold(false);
    title_->setFont(f);

	verticalLayout_2->addWidget(title_);
	
	auto horizontalLayout_9 = new QHBoxLayout();
	horizontalLayout_9->setSpacing(6);
	horizontalLayout_9->setObjectName(QString::fromUtf8("horizontalLayout_9"));
	auto horizontalLayout_8 = new QHBoxLayout();
	horizontalLayout_8->setSpacing(0);
	horizontalLayout_8->setObjectName(QString::fromUtf8("horizontalLayout_8"));
	auto label_3 = new QLabel(this);
	label_3->setObjectName(QString::fromUtf8("label_3"));

	label_3->setText(tr("Artist:"));
	label_3->setFont(f);
	label_3->setMinimumHeight(40);
	label_3->setMinimumWidth(60);
    label_3->setStyleSheet(Q_UTF8("background-color: transparent"));
	horizontalLayout_8->addWidget(label_3);

	artist_ = new ScrollLabel(this);
	artist_->setObjectName(QString::fromUtf8("label_5"));
    artist_->setStyleSheet(Q_UTF8("background-color: transparent"));
	artist_->setFont(f);

	horizontalLayout_8->addWidget(artist_);

	horizontalLayout_8->setStretch(1, 0);

	horizontalLayout_9->addLayout(horizontalLayout_8);

	auto horizontalLayout_7 = new QHBoxLayout();
	horizontalLayout_7->setSpacing(0);
	horizontalLayout_7->setObjectName(QString::fromUtf8("horizontalLayout_7"));
	auto label_7 = new QLabel(this);	
	label_7->setObjectName(QString::fromUtf8("label_4"));
	label_7->setText(tr("Album:"));

	label_7->setMinimumWidth(60);
	label_7->setFont(f);
    label_7->setStyleSheet(Q_UTF8("background-color: transparent"));
	horizontalLayout_7->addWidget(label_7);

	album_ = new ScrollLabel(this);
	album_->setObjectName(QString::fromUtf8("label_6"));
    album_->setStyleSheet(Q_UTF8("background-color: transparent"));
	album_->setFont(f);

	horizontalLayout_7->addWidget(album_);

	horizontalLayout_7->setStretch(1, 0);

	horizontalLayout_9->addLayout(horizontalLayout_7);
	

	verticalLayout_2->addLayout(horizontalLayout_9);

	lyrics_widget_ = new LyricsShowWideget(this);
	lyrics_widget_->setObjectName(QString::fromUtf8("lyrics"));
	lyrics_widget_->setMinimumSize(QSize(200, 60));	
	verticalLayout_2->addWidget(lyrics_widget_);

	verticalLayout_2->setStretch(2, 1);

	horizontalLayout_10->addLayout(verticalLayout_2);
}
