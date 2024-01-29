#include <widget/lrcpage.h>

#include <QHBoxLayout>
#include <QGraphicsDropShadowEffect>
#include <QPainter>

#include <thememanager.h>

#include <widget/acrylic/acrylic.h>
#include <widget/spectrumwidget.h>
#include <widget/image_utiltis.h>
#include <widget/scrolllabel.h>
#include <widget/lyricsshowwidget.h>
#include <widget/str_utilts.h>
#include <widget/appsettingnames.h>
#include <widget/appsettings.h>
#include <widget/seekslider.h>

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

void LrcPage::addCoverShadow(bool found_cover) {
	cover_label_->setGraphicsEffect(nullptr);

	if (found_cover) {
		auto* effect = new QGraphicsDropShadowEffect(this);
		effect->setOffset(5, 10);
		effect->setColor(QColor(qTEXT("#080808")));
		effect->setBlurRadius(40);
		cover_label_->setGraphicsEffect(effect);
	}
}

void LrcPage::setCover(const QPixmap& src) {
    cover_ = src.copy();
    //SetFullScreen(spectrum_->width() > 700);
	setFullScreen(false);
	addCoverShadow(true);
}

QSize LrcPage::coverSize() const {
	return coverSizeHint();
}

QLabel* LrcPage::format() {
	return format_label_;
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

void LrcPage::setFullScreen(bool enter) {
	auto f = font();

	if (!enter) {
		cover_label_->setMinimumSize(QSize(411, 411));
		cover_label_->setMaximumSize(QSize(411, 411));
		f.setPointSize(qTheme.fontSize(12));
		format_label_->setFont(f);
	} else {
		cover_label_->setMinimumSize(QSize(822, 822));
		cover_label_->setMaximumSize(QSize(822, 822));
		f.setPointSize(qTheme.fontSize(16));
		format_label_->setFont(f);
	}

    cover_label_->setPixmap(
		image_utils::roundImage(image_utils::resizeImage(cover_, coverSizeHint(), false),
		image_utils::kSmallImageRadius));
}

QSize LrcPage::coverSizeHint() const {
	const QSize cover_size(cover_label_->size().width() - image_utils::kSmallImageRadius,
		cover_label_->size().height() - image_utils::kSmallImageRadius);
	return cover_size;
}

void LrcPage::resizeEvent(QResizeEvent* event) {
	//SetFullScreen(spectrum_->width() > 700);
	setFullScreen(false);
}

void LrcPage::setBackground(const QImage& cover) {
	if (cover.isNull()) {
		background_image_ = QImage();
	}
	else {
		prev_bg_alpha_ = current_bg_alpha_;
		prev_background_image_ = cover;
        background_image_ = cover;
		startBackgroundAnimation(kBlurBackgroundAnimationMs);
	}
	update();
}

void LrcPage::startBackgroundAnimation(const int durationMs) {
	current_bg_alpha_ = image_utils::sampleImageBlur(background_image_, kBlurAlpha);

	auto* fade_in_animation = new QPropertyAnimation(this, "appearBgProg");
	fade_in_animation->setStartValue(0);
	fade_in_animation->setEndValue(current_bg_alpha_);
	fade_in_animation->setDuration(durationMs);
	fade_in_animation->setEasingCurve(QEasingCurve::OutCubic);
	(void)QObject::connect(fade_in_animation, &QPropertyAnimation::finished, this, [fade_in_animation] {
		fade_in_animation->deleteLater();
		});
	current_bg_alpha_ = 0;
	fade_in_animation->start();

	auto* fade_out_animation = new QPropertyAnimation(this, "disappearBgProg");
	fade_out_animation->setStartValue(prev_bg_alpha_);
	fade_out_animation->setEndValue(0);
	fade_out_animation->setDuration(durationMs);
	fade_out_animation->setEasingCurve(QEasingCurve::OutCubic);
	(void)QObject::connect(fade_out_animation, &QPropertyAnimation::finished, this, [this, fade_out_animation] {
		prev_background_image_ = QImage();
		fade_out_animation->deleteLater();
		update();
		});
	fade_out_animation->start();
}

void LrcPage::paintEvent(QPaintEvent*) {
	QPainter painter(this);

	if (background_image_.isNull()) {
		return;
	}

	painter.setCompositionMode(QPainter::CompositionMode_Overlay);

	if (!background_image_.isNull()) {
		painter.setOpacity(current_bg_alpha_ / 255.0);
		painter.drawImage(rect(), background_image_);
		painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
	}
	if (!prev_background_image_.isNull() && prev_bg_alpha_) {
		painter.setOpacity(prev_bg_alpha_ / 255.0);
		painter.drawImage(rect(), prev_background_image_);
		painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
	}
}

void LrcPage::setAppearBgProgress(int x) {
	current_bg_alpha_ = x;
	update();
}

int LrcPage::getAppearBgProgress() const {
	return current_bg_alpha_;
}

void LrcPage::setDisappearBgProgress(int x) {
	prev_bg_alpha_ = x;
	update();
}

int LrcPage::getDisappearBgProgress() const {
	return prev_bg_alpha_;
}

void LrcPage::onThemeChangedFinished(ThemeColor theme_color) {
	cover_label_->setGraphicsEffect(nullptr);

	if (theme_color == ThemeColor::LIGHT_THEME) {
		auto* effect = new QGraphicsDropShadowEffect(this);
		effect->setOffset(10, 20);
		effect->setColor(qTheme.coverShadowColor());
		effect->setBlurRadius(50);
		cover_label_->setGraphicsEffect(effect);
	}

	switch (theme_color) {
	case ThemeColor::DARK_THEME:
		lyrics_widget_->onSetLrcColor(Qt::lightGray);
		lyrics_widget_->onSetLrcHighLight(Qt::white);
		qAppSettings.setValue(kLyricsTextColor, QColor(Qt::lightGray));
		qAppSettings.setValue(kLyricsHighLightTextColor, QColor(Qt::white));
		break;
	case ThemeColor::LIGHT_THEME:
		lyrics_widget_->onSetLrcColor(Qt::white);
		lyrics_widget_->onSetLrcHighLight(Qt::black);
		qAppSettings.setValue(kLyricsTextColor, QColor(Qt::lightGray));
		qAppSettings.setValue(kLyricsHighLightTextColor, QColor(Qt::black));
		break;
	}
}

void LrcPage::onThemeColorChanged(QColor background_color, QColor color) {	
	onThemeChangedFinished(qTheme.themeColor());
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

	format_label_ = new QLabel(this);
	format_label_->setObjectName(QString::fromUtf8("formatCoverLabel"));
	format_label_->setMinimumHeight(40);
	format_label_->setMaximumHeight(40);
	format_label_->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	QFont format_font(qTEXT("FormatFont"));
	format_font.setPointSize(qTheme.fontSize(12));
	format_label_->setFont(format_font);

    cover_label_ = new QLabel(this);
    cover_label_->setObjectName(QString::fromUtf8("lrcCoverLabel"));
	cover_label_->setMinimumSize(QSize(411, 411));
    cover_label_->setMaximumSize(QSize(411, 411));
	cover_label_->setStyleSheet(qTEXT("background-color: transparent"));
	cover_label_->setAttribute(Qt::WA_StaticContents);

    vertical_layout_3->addWidget(cover_label_);

	auto* horizontalLayout_4 = new QHBoxLayout();
	horizontalLayout_4->setSpacing(0);
	horizontalLayout_4->setObjectName(QString::fromUtf8("horizontalLayout_4"));
	horizontalLayout_4->setContentsMargins(-1, 3, -1, 3);
	auto* endPosLabel = new QLabel();
	endPosLabel->setObjectName(QString::fromUtf8("endPosLabel"));
	endPosLabel->setMinimumSize(QSize(50, 20));
	endPosLabel->setMaximumSize(QSize(50, 20));
	endPosLabel->setAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
	endPosLabel->setMargin(1);

	horizontalLayout_4->addWidget(endPosLabel);

	auto* seekSlider = new SeekSlider();
	seekSlider->setObjectName(QString::fromUtf8("seekSlider"));
	seekSlider->setMinimumSize(QSize(300, 0));
	seekSlider->setMaximumSize(QSize(300, 16));
	seekSlider->setOrientation(Qt::Horizontal);

	horizontalLayout_4->addWidget(seekSlider);

	auto* startPosLabel = new QLabel();
	startPosLabel->setObjectName(QString::fromUtf8("startPosLabel"));
	startPosLabel->setMinimumSize(QSize(50, 20));
	startPosLabel->setMaximumSize(QSize(50, 20));

	horizontalLayout_4->addWidget(startPosLabel);

	vertical_layout_3->addLayout(horizontalLayout_4);
	vertical_layout_3->addWidget(format_label_);
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
	f.setPointSize(qTheme.fontSize(12));
	f.setBold(true);

    title_ = new ScrollLabel(this);
	title_->setStyleSheet(qTEXT("background-color: transparent"));
	title_->setObjectName(QString::fromUtf8("label_2"));
	title_->setText(tr("Title:"));
	title_->setFont(f);
    title_->setMinimumHeight(40);
	f.setPointSize(qTheme.fontSize(15));
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

	startPosLabel->hide();
	endPosLabel->hide();
	seekSlider->hide();

	label_3->hide();
	label_7->hide();
	title_->hide();
	album_->hide();
	artist_->hide();

	setStyleSheet(qTEXT("background-color: transparent"));
}
