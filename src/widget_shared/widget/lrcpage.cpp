#include <widget/lrcpage.h>

#include <QHBoxLayout>
#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QHeaderView>
#include <QSaveFile>

#include <thememanager.h>

#include <widget/spectrumwidget.h>
#include <widget/util/image_util.h>
#include <widget/scrolllabel.h>
#include <widget/lyricsshowwidget.h>
#include <widget/util/str_util.h>
#include <widget/appsettings.h>
#include <widget/seekslider.h>
#include <widget/util/ui_util.h>

LyricsFrame::LyricsFrame(QWidget* parent)
	: QFrame(parent) {
	setAttribute(Qt::WA_DontCreateNativeAncestors);
	//setFrameShape(QFrame::StyledPanel);
	//setFrameShadow(QFrame::Raised);

	lyrc_list_ = new QTableWidget(this);

	QStringList headers;
	headers << tr("Type")
	<< tr("Album")
	<< tr("Artist")
	<< tr("Title")
	<< tr("Lyrics")
	<< tr("Karaoke")
	<< tr("Duration");
	lyrc_list_->setHorizontalHeaderLabels(headers);
	lyrc_list_->setColumnCount(headers.count());

	lyrc_list_->setEditTriggers(QTableWidget::NoEditTriggers);
	lyrc_list_->setSelectionBehavior(QAbstractItemView::SelectRows);
	lyrc_list_->setSelectionMode(QAbstractItemView::SingleSelection);
	lyrc_list_->horizontalHeader()->setStretchLastSection(true);
	lyrc_list_->horizontalHeader()->hide();
	lyrc_list_->setColumnWidth(4, 400);
	lyrc_list_->setColumnWidth(5, 80);

	lyrc_list_->verticalHeader()->setVisible(false);
	lyrc_list_->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
	lyrc_list_->verticalHeader()->setDefaultSectionSize(240);

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->addWidget(lyrc_list_);
	setLayout(layout);

	(void) QObject::connect(lyrc_list_, &QTableWidget::itemDoubleClicked,
		[this](auto *item) {
		auto* find_item = lyrc_list_->item(item->row(), 0);
		auto id = find_item->data(Qt::UserRole).toString();
		auto candidate = parser_map_.value(id);
		emit changeLyric(candidate);
		});

	setStyleSheet("background-color: transparent"_str);
	lyrc_list_->setStyleSheet("background-color: transparent"_str);
}

void LyricsFrame::setLyrics(const QList<SearchLyricsResult>& results) {
	lyrc_list_->clearContents();

	for (auto& [info, parsers] : results) {
		for (auto& candidate : parsers) {
			QString typeStr = tr("Original");
			if (candidate.parser->hasTranslation()) {
				typeStr = tr("Translation");
			}
			if (candidate.parser->size() == 0) {
				continue;
			}
			auto* typeItem = new QTableWidgetItem(typeStr);
			QStringList lyricText;
			for (auto i = 0; i < 5 && i < candidate.parser->size(); ++i) {
				lyricText << QString::fromStdWString(candidate.parser->lineAt(i).lrc);
				if (candidate.parser->hasTranslation()) {
					lyricText << QString::fromStdWString(candidate.parser->lineAt(i).tlrc);
				}
			}
			auto* lyricItem = new QTableWidgetItem(lyricText.join("\r\n"_str));
			auto* durationItem = new QTableWidgetItem(
				formatDuration(
					static_cast<double>(candidate.parser->last().timestamp.count()) / 1000.0));
			auto* karaokeItem = new QTableWidgetItem(
				candidate.parser->isKaraoke() ? tr("Yes") : tr("No"));
			auto* titleItem = new QTableWidgetItem(candidate.candidate.song);
			auto* artistItem = new QTableWidgetItem(candidate.candidate.singer);
			auto* albumItem = new QTableWidgetItem(candidate.candidate.albumName);
			lyrc_list_->insertRow(lyrc_list_->rowCount());
			lyrc_list_->setItem(lyrc_list_->rowCount() - 1, 0, typeItem);
			lyrc_list_->setItem(lyrc_list_->rowCount() - 1, 1, albumItem);
			lyrc_list_->setItem(lyrc_list_->rowCount() - 1, 2, titleItem);
			lyrc_list_->setItem(lyrc_list_->rowCount() - 1, 3, artistItem);
			lyrc_list_->setItem(lyrc_list_->rowCount() - 1, 4, lyricItem);
			lyrc_list_->setItem(lyrc_list_->rowCount() - 1, 5, karaokeItem);
			lyrc_list_->setItem(lyrc_list_->rowCount() - 1, 6, durationItem);
			auto id = QString::fromLatin1(generateUuid());
			typeItem->setData(Qt::UserRole, id);
			parser_map_.insert(id, candidate);
		}
	}
}

LrcPage::LrcPage(QWidget* parent)
	: QFrame(parent) {
	setObjectName("lrcPage"_str);
	setFrameShape(QFrame::StyledPanel);
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
		effect->setColor(QColor("#080808"_str));
		effect->setBlurRadius(40);
		cover_label_->setGraphicsEffect(effect);
	}
}

void LrcPage::setCover(const QPixmap& src) {
    cover_ = src.copy();
	setFullScreen();
#ifndef _DEBUG
	addCoverShadow(true);
#endif
}

void LrcPage::setPlayListEntity(const PlayListEntity& entity) {
	entity_ = entity;
	lyrics_results_.clear();
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

void LrcPage::onFetchLyricsCompleted(const QList<SearchLyricsResult>& results) {
	lyrics_results_ = results;

	if (results.isEmpty()) {
		return;
	}

	const auto hasTranslation = [](const SearchLyricsResult& res) ->bool {
		for (auto& lyricsParser : res.parsers) {
			if (lyricsParser.parser && lyricsParser.parser->hasTranslation()) {
				return true;
			}
		}
		return false;
		};

	std::sort(lyrics_results_.begin(), lyrics_results_.end(),
		[hasTranslation](auto& lhs, auto& rhs) {
		bool lhsHas = hasTranslation(lhs);
		bool rhsHas = hasTranslation(rhs);
		return (lhsHas > rhsHas);
		});

	for (const auto& [info, parsers] : lyrics_results_) {
		for (auto &parser : parsers) {
			if (parser.parser->hasTranslation()) {
				if (parser.candidate.song == entity_.title
					|| parser.candidate.singer == entity_.artist) {
					lyrics_widget_->loadFromParser(parser.parser);
					auto krc_file_name = entity_.parent_path
					+ "\\"_str + entity_.file_name + ".krc"_str;
					QSaveFile file(krc_file_name);
					file.open(QIODevice::WriteOnly);
					file.write(parser.content);
					if (file.commit()) {
						lyrics_widget_->loadFile(krc_file_name);
					}
					return;
				}
			}
		}
	}
}

void LrcPage::setFullScreen() {
	auto enter = false;
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
		image_util::roundImage(image_util::resizeImage(cover_, coverSizeHint(), false),
		image_util::kSmallImageRadius));
}

QSize LrcPage::coverSizeHint() const {
	const QSize cover_size(cover_label_->size().width() - image_util::kSmallImageRadius,
		cover_label_->size().height() - image_util::kSmallImageRadius);
	return cover_size;
}

void LrcPage::resizeEvent(QResizeEvent* event) {
	setFullScreen();
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
	current_bg_alpha_ = image_util::sampleImageBlur(background_image_, kBlurAlpha);

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

	painter.fillRect(rect(), QColor("#121212"));

	if (background_image_.isNull()) {
		return;
	}

	painter.setCompositionMode(QPainter::CompositionMode_Overlay);
#ifndef _DEBUG
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
#endif
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
		lyrics_widget_->setNormalColor(Qt::lightGray);
		lyrics_widget_->setHighLightColor(Qt::white);
		break;
	case ThemeColor::LIGHT_THEME:
		lyrics_widget_->setNormalColor(Qt::darkGray);
		lyrics_widget_->setHighLightColor(Qt::black);
		break;
	}
}

void LrcPage::initial() {
	auto* main_layout = new QVBoxLayout(this);
	main_layout->setSpacing(0);
	main_layout->setObjectName(QString::fromUtf8("default_layout"));
	main_layout->setContentsMargins(0, 0, 0, 0);

	auto horizontal_layout_10 = new QHBoxLayout();
	
	horizontal_layout_10->setSpacing(0);
	horizontal_layout_10->setObjectName(QString::fromUtf8("horizontalLayout_10"));
	horizontal_layout_10->setContentsMargins(80, 80, 80, 80);
	horizontal_layout_10->setStretch(0, 0);
	horizontal_layout_10->setStretch(0, 1);

	auto vertical_layout_3 = new QVBoxLayout();
	vertical_layout_3->setSpacing(0);
	vertical_layout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));

	format_label_ = new QLabel(this);
	format_label_->setObjectName(QString::fromUtf8("formatCoverLabel"));
	format_label_->setMinimumHeight(40);
	format_label_->setMaximumHeight(40);
	format_label_->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	format_label_->setStyleSheet("background-color: transparent"_str);
	QFont format_font("FormatFont"_str);
	format_font.setPointSize(qTheme.fontSize(12));
	format_label_->setFont(format_font);

    cover_label_ = new QLabel(this);
    cover_label_->setObjectName(QString::fromUtf8("lrcCoverLabel"));
	cover_label_->setMinimumSize(QSize(250, 250));
    cover_label_->setMaximumSize(QSize(250, 250));
	cover_label_->setStyleSheet("background-color: transparent"_str);
	cover_label_->setAttribute(Qt::WA_StaticContents);

    vertical_layout_3->addWidget(cover_label_);

	auto* horizontalLayout_4 = new QHBoxLayout();
	horizontalLayout_4->setSpacing(0);
	horizontalLayout_4->setObjectName(QString::fromUtf8("horizontalLayout_4"));
	horizontalLayout_4->setContentsMargins(-1, 3, -1, 3);
	/*auto* endPosLabel = new QLabel();
	endPosLabel->setObjectName(QString::fromUtf8("endPosLabel"));
	endPosLabel->setMinimumSize(QSize(50, 20));
	endPosLabel->setMaximumSize(QSize(50, 20));
	endPosLabel->setAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
	endPosLabel->setMargin(1);

	horizontalLayout_4->addWidget(endPosLabel);

	auto* seekSlider = new SeekSlider();
	seekSlider->setObjectName(QString::fromUtf8("seekSlider"));
	seekSlider->setMinimumSize(QSize(250, 0));
	seekSlider->setMaximumSize(QSize(250, 16));
	seekSlider->setOrientation(Qt::Horizontal);

	horizontalLayout_4->addWidget(seekSlider);

	auto* startPosLabel = new QLabel();
	startPosLabel->setObjectName(QString::fromUtf8("startPosLabel"));
	startPosLabel->setMinimumSize(QSize(50, 20));
	startPosLabel->setMaximumSize(QSize(50, 20));

	horizontalLayout_4->addWidget(startPosLabel);*/

	vertical_layout_3->addLayout(horizontalLayout_4);
	vertical_layout_3->addWidget(format_label_);
	vertical_layout_3->setContentsMargins(0, 20, 0, 0);

	auto vertical_spacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

	vertical_layout_3->addItem(vertical_spacer);

	horizontal_layout_10->addLayout(vertical_layout_3);

	auto horizontal_spacer_4 = new QSpacerItem(10, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

	horizontal_layout_10->addItem(horizontal_spacer_4);

	auto vertical_layout_2 = new QVBoxLayout();

	vertical_layout_2->setSpacing(0);
	vertical_layout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));

	auto f = font();
	f.setPointSize(qTheme.fontSize(12));
	f.setBold(true);

    title_ = new ScrollLabel(this);
	title_->setStyleSheet("background-color: transparent"_str);
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
    label_3->setStyleSheet("background-color: transparent; color: gray;"_str);
	horizontalLayout_8->addWidget(label_3);

	artist_ = new ScrollLabel(this);
	artist_->setObjectName(QString::fromUtf8("label_5"));
    artist_->setStyleSheet("background-color: transparent"_str);
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
    label_7->setStyleSheet("background-color: transparent; color: gray;"_str);
	horizontal_layout_7->addWidget(label_7);

	album_ = new ScrollLabel(this);
	album_->setObjectName(QString::fromUtf8("label_6"));
    album_->setStyleSheet("background-color: transparent"_str);
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
	spectrum_->setStyleSheet("background-color: transparent"_str);
	vertical_layout_2->addWidget(spectrum_);

	auto horizontal_layout_11 = new QHBoxLayout();
	auto* change_lrc_button = new QToolButton(this);
	change_lrc_button->setText(tr("Change LRC"));
	(void)QObject::connect(change_lrc_button, &QToolButton::clicked, [this]() {
		if (lyrics_results_.isEmpty()) {
			return;
		}
		const QScopedPointer<XDialog> dialog(new XDialog(this));
		const QScopedPointer<LyricsFrame> lyrics_frame(new LyricsFrame(dialog.get()));
		dialog->setTitle(tr("Change LRC"));
		lyrics_frame->setLyrics(lyrics_results_);
		dialog->setContentWidget(lyrics_frame.get());
		dialog->setFixedSize(QSize(1000, 600));
		dialog->setIcon(qTheme.fontIcon(Glyphs::ICON_SUBTITLE));
		(void)QObject::connect(lyrics_frame.get(),
			&LyricsFrame::changeLyric,
			[this](const LyricsParser& parser) {
			auto krc_file_name = entity_.parent_path + "\\"_str + entity_.file_name + ".krc"_str;
			QSaveFile file(krc_file_name);
			file.open(QIODevice::WriteOnly);
			file.write(parser.content);
			if (file.commit()) {
				lyrics_widget_->loadFile(krc_file_name);
			}
			});
		dialog->exec();
		});

	auto horizontal_spacer_5 = new QSpacerItem(10, 20, QSizePolicy::Expanding, QSizePolicy::Fixed);
	horizontal_layout_11->addWidget(change_lrc_button);
	horizontal_layout_11->addSpacerItem(horizontal_spacer_5);
	vertical_layout_2->addLayout(horizontal_layout_11);

	vertical_layout_2->setStretch(2, 1);

	horizontal_layout_10->addLayout(vertical_layout_2);
	horizontal_layout_10->setStretch(2, 1);

	//startPosLabel->hide();
	//endPosLabel->hide();
	//seekSlider->hide();

	label_3->hide();
	label_7->hide();
	title_->hide();
	album_->hide();
	artist_->hide();

	//lyrics_widget_->hide();
	//cover_label_->hide();
	spectrum_->hide();

	main_layout->addLayout(horizontal_layout_10);
	setStyleSheet("background-color: transparent; border: none;"_str);
}
