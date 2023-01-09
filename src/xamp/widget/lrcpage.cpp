#include <QLabel>
#include <QHBoxLayout>
#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QPropertyAnimation>

#include "thememanager.h"

#include <widget/spectrumwidget.h>
#include <widget/image_utiltis.h>
#include <widget/scrolllabel.h>
#include <widget/lyricsshowwidget.h>
#include <widget/str_utilts.h>
#include <widget/lrcpage.h>

#include "appsettingnames.h"
#include "appsettings.h"

LrcPage::LrcPage(QWidget* parent)
	: QFrame(parent) {
	setObjectName(qTEXT("lrcPage"));
	initial();
}

LyricsShowWidget* LrcPage::lyrics() {
	return lyrics_widget_;
}

QLabel* LrcPage::cover() {
    return cover_label_;
}

SpectrumWidget* LrcPage::spectrum() {
	return spectrum_;
}

void LrcPage::setCover(const QPixmap& src) {
    auto cover_size = cover_label_->size();
    cover_label_->setPixmap(ImageUtils::roundImage(src, QSize(cover_size.width() - 5, cover_size.height() - 5), 5));
}

QSize LrcPage::coverSize() const {
	return cover_label_->size();
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

void LrcPage::clearBackground() {
	background_image_ = QImage();
	update();
}

void LrcPage::setBackground(const QImage& cover) {
	if (cover.isNull()) {
		background_image_ = QImage();
	}
	else {
		prev_bg_alpha_ = current_bg_alpha_;
		prev_background_image_ = cover;
        background_image_ = cover;
		constexpr int kBlurBackgroundAnimationMs = 3000;
		startBackgroundAnimation(kBlurBackgroundAnimationMs);
	}
	update();
}

void LrcPage::startBackgroundAnimation(const int durationMs) {
	constexpr auto kBlurAlpha = 128;
	current_bg_alpha_ = ImageUtils::sampleImageBlur(background_image_, kBlurAlpha);

	auto* fade_in_animation = new QPropertyAnimation(this, "appearBgProg");
	fade_in_animation->setStartValue(0);
	fade_in_animation->setEndValue(current_bg_alpha_);
	fade_in_animation->setDuration(durationMs);
	fade_in_animation->setEasingCurve(QEasingCurve::OutCubic);
	QObject::connect(fade_in_animation, &QPropertyAnimation::finished, this, [=] {
		fade_in_animation->deleteLater();
		});
	current_bg_alpha_ = 0;
	fade_in_animation->start();

	auto* fade_out_animation = new QPropertyAnimation(this, "disappearBgProg");
	fade_out_animation->setStartValue(prev_bg_alpha_);
	fade_out_animation->setEndValue(0);
	fade_out_animation->setDuration(durationMs);
	fade_out_animation->setEasingCurve(QEasingCurve::OutCubic);
	QObject::connect(fade_out_animation, &QPropertyAnimation::finished, this, [=] {
		prev_background_image_ = QImage();
		fade_out_animation->deleteLater();
		update();
		});
	fade_out_animation->start();
}

void LrcPage::paintEvent(QPaintEvent*) {
	if (AppSettings::getValueAsBool(kEnableBlurCover)) {
		cover_label_->setGraphicsEffect(nullptr);
	}
	QPainter painter(this);
	if (!background_image_.isNull()) {
		painter.setOpacity(current_bg_alpha_ / 255.0);
		painter.drawImage(rect(), background_image_);
	}
	if (!prev_background_image_.isNull() && prev_bg_alpha_) {
		painter.setOpacity(prev_bg_alpha_ / 255.0);
		painter.drawImage(rect(), prev_background_image_);
	}
}

void LrcPage::setAppearBgProg(int x) {
	current_bg_alpha_ = x;
	update();
}

int LrcPage::getAppearBgProg() const {
	return current_bg_alpha_;
}

void LrcPage::setDisappearBgProg(int x) {
	prev_bg_alpha_ = x;
	update();
}

int LrcPage::getDisappearBgProg() const {
	return prev_bg_alpha_;
}

void LrcPage::setBackgroundColor(QColor backgroundColor) {
    //lyrics_widget_->setBackgroundColor(backgroundColor);
	//setStyleSheet(backgroundColorToString(backgroundColor));
}

void LrcPage::onThemeChanged(QColor backgroundColor, QColor color) {
    //lyrics_widget_->setLrcColor(color);
    //lyrics_widget_->setLrcHightLight(color);
    //lyrics_widget_->setLrcColor(Qt::white);
    //lyrics_widget_->setLrcHightLight(Qt::white);
}

void LrcPage::initial() {
	auto horizontal_layout_10 = new QHBoxLayout(this);
	
	horizontal_layout_10->setSpacing(0);
	horizontal_layout_10->setObjectName(QString::fromUtf8("horizontalLayout_10"));
	horizontal_layout_10->setContentsMargins(80, 80, 80, 80);
	horizontal_layout_10->setStretch(1, 1);

	auto vertical_layout_3 = new QVBoxLayout();
	vertical_layout_3->setSpacing(0);
	vertical_layout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));	

    cover_label_ = new QLabel(this);
    cover_label_->setObjectName(QString::fromUtf8("lrcCoverLabel"));
#ifdef Q_OS_WIN
	cover_label_->setMinimumSize(QSize(355, 355));
    cover_label_->setMaximumSize(QSize(355, 355));
#else
    cover_label_->setMinimumSize(QSize(500, 500));
    cover_label_->setMaximumSize(QSize(500, 500));
#endif
	cover_label_->setStyleSheet(qTEXT("background-color: transparent"));
	cover_label_->setAttribute(Qt::WA_StaticContents);

	if (!AppSettings::getValueAsBool(kEnableBlurCover)) {
		auto* effect = new QGraphicsDropShadowEffect(this);
		effect->setOffset(10, 20);
		effect->setColor(qTheme.coverShadownColor());
		effect->setBlurRadius(50);
		cover_label_->setGraphicsEffect(effect);
	}

    vertical_layout_3->addWidget(cover_label_);
	vertical_layout_3->setContentsMargins(0, 20, 0, 0);

	auto vertical_spacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

	vertical_layout_3->addItem(vertical_spacer);

	horizontal_layout_10->addLayout(vertical_layout_3);

	auto horizontal_spacer_4 = new QSpacerItem(50, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

	horizontal_layout_10->addItem(horizontal_spacer_4);

	auto vertical_layout_2 = new QVBoxLayout();

	vertical_layout_2->setSpacing(0);
	vertical_layout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));

	auto f = font();
    f.setPointSize(12);
	f.setBold(true);

    title_ = new ScrollLabel(this);
	title_->setStyleSheet(qTEXT("background-color: transparent"));
	title_->setObjectName(QString::fromUtf8("label_2"));
	title_->setText(tr("Title:"));	
	title_->setFont(f);
#ifdef Q_OS_WIN
    title_->setMinimumHeight(40);
    f.setPointSize(15);
#else
    title_->setMinimumHeight(50);
    f.setPointSize(25);
#endif
    f.setBold(false);
    title_->setFont(f);

	vertical_layout_2->addWidget(title_);
	
	auto horizontal_layout_9 = new QHBoxLayout();
	horizontal_layout_9->setSpacing(6);
	horizontal_layout_9->setObjectName(QString::fromUtf8("horizontalLayout_9"));
	auto horizontalLayout_8 = new QHBoxLayout();
	horizontalLayout_8->setSpacing(0);
	horizontalLayout_8->setObjectName(QString::fromUtf8("horizontalLayout_8"));
	auto label_3 = new QLabel(this);
	label_3->setObjectName(QString::fromUtf8("label_3"));

	label_3->setText(tr("Artist:"));
	label_3->setFont(f);
	label_3->setMinimumHeight(40);
	label_3->setMinimumWidth(60);
    label_3->setStyleSheet(qTEXT("background-color: transparent; color: gray;"));
	horizontalLayout_8->addWidget(label_3);

	artist_ = new ScrollLabel(this);
	artist_->setObjectName(QString::fromUtf8("label_5"));
    artist_->setStyleSheet(qTEXT("background-color: transparent"));
	artist_->setFont(f);

	horizontalLayout_8->addWidget(artist_);

	horizontalLayout_8->setStretch(1, 0);

	horizontal_layout_9->addLayout(horizontalLayout_8);

	auto horizontal_layout_7 = new QHBoxLayout();
	horizontal_layout_7->setSpacing(0);
	horizontal_layout_7->setObjectName(QString::fromUtf8("horizontalLayout_7"));
	auto label_7 = new QLabel(this);	
	label_7->setObjectName(QString::fromUtf8("label_4"));
	label_7->setText(tr("Album:"));

	label_7->setMinimumWidth(50);
	label_7->setFont(f);
    label_7->setStyleSheet(qTEXT("background-color: transparent; color: gray;"));
	horizontal_layout_7->addWidget(label_7);

	album_ = new ScrollLabel(this);
	album_->setObjectName(QString::fromUtf8("label_6"));
    album_->setStyleSheet(qTEXT("background-color: transparent"));
	album_->setFont(f);

	horizontal_layout_7->addWidget(album_);

	horizontal_layout_7->setStretch(1, 0);

	horizontal_layout_9->addLayout(horizontal_layout_7);
	

	vertical_layout_2->addLayout(horizontal_layout_9);

	lyrics_widget_ = new LyricsShowWidget(this);
	lyrics_widget_->setObjectName(QString::fromUtf8("lyrics"));
	lyrics_widget_->setMinimumSize(QSize(180, 60));
	vertical_layout_2->addWidget(lyrics_widget_);

	spectrum_ = new SpectrumWidget(this);
	spectrum_->setMinimumSize(QSize(180, 60));
	spectrum_->setStyleSheet(qTEXT("background-color: transparent"));
	vertical_layout_2->addWidget(spectrum_);

	vertical_layout_2->setStretch(2, 1);

	horizontal_layout_10->addLayout(vertical_layout_2);
	horizontal_layout_10->setStretch(1, 1);

	label_3->hide();
	label_7->hide();
	title_->hide();
	album_->hide();
	artist_->hide();

	setStyleSheet(qTEXT("background-color: transparent"));
}
