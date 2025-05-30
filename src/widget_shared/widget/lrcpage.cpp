﻿#include <widget/lrcpage.h>

#include <QHBoxLayout>
#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QHeaderView>
#include <QSaveFile>
#include <QStandardItemModel>
#include <QColorDialog>
#include <QDir>
#include <QSortFilterProxyModel>
#include <QLineEdit>

#include <thememanager.h>

#include <widget/spectrumwidget.h>
#include <widget/util/image_util.h>
#include <widget/scrolllabel.h>
#include <widget/lyricsshowwidget.h>
#include <widget/util/str_util.h>
#include <widget/appsettings.h>
#include <widget/seekslider.h>
#include <widget/util/ui_util.h>
#include <xampplayer.h>

LyricsFrame::LyricsFrame(QWidget* parent)
	: QFrame(parent) {
	setAttribute(Qt::WA_DontCreateNativeAncestors);
	setFrameShape(QFrame::StyledPanel);
	setFrameShadow(QFrame::Raised);

	lyrc_view_ = new QTableView(this);
	model_ = new QStandardItemModel(this);

	QStringList headers;
	headers << tr("Type")
	<< tr("Album")
	<< tr("Artist")
	<< tr("Title")
	<< tr("Lyric preview")
	<< tr("Has karaoke")
	<< tr("Duration");
	model_->setColumnCount(headers.count());
	model_->setHorizontalHeaderLabels(headers);

	QLineEdit* searchEdit = new QLineEdit(this);
	searchEdit->setPlaceholderText(tr("Search by Album / Artist / Title"));
	filterModel_ = new MultiColumnFilterModel(this);
	filterModel_->setSourceModel(model_);
	// 若希望動態過濾 (輸入即更新)
	filterModel_->setDynamicSortFilter(true);

	//lyrc_view_->setModel(model_);
	lyrc_view_->setModel(filterModel_);

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->addWidget(searchEdit);
	layout->addWidget(lyrc_view_);
	setLayout(layout);

	(void) QObject::connect(lyrc_view_, &QTableView::doubleClicked,
		[this](const QModelIndex& index) {
		if (!index.isValid()) {
			return;
		}
		QModelIndex srcIndex = filterModel_->mapToSource(index);
		QModelIndex idIndex = model_->index(srcIndex.row(), 0);
		QVariant id = model_->data(idIndex, Qt::UserRole);
		auto candidate = parser_map_.value(id.toString());
		emit changeLyric(candidate);
		});

	(void)QObject::connect(searchEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
		QRegularExpression re(text, QRegularExpression::CaseInsensitiveOption);
		filterModel_->setFilterRegularExpression(re);
		});

	setTabViewStyle(lyrc_view_);

	lyrc_view_->setColumnWidth(4, 400);
	lyrc_view_->setColumnWidth(5, 80);
	lyrc_view_->verticalHeader()->setDefaultSectionSize(240);
	lyrc_view_->setEditTriggers(QTableWidget::NoEditTriggers);
}

void LyricsFrame::setLyrics(const QList<SearchLyricsResult>& results) {
	// 先清理舊資料
	model_->removeRows(0, model_->rowCount());
	parser_map_.clear();

	// 遍歷你的 results
	for (auto& [info, result] : results) {
		for (auto& lrc_parser : result) {
			QString typeStr = tr("Original");
			if (lrc_parser.parser->hasTranslation()) {
				typeStr = tr("Translation");
			}
			if (lrc_parser.parser->size() == 0) {
				continue;
			}

			// 準備欄位資料
			QStringList lyricText;
			for (auto i = 0; i < 5 && i < lrc_parser.parser->size(); ++i) {
				lyricText << QString::fromStdWString(lrc_parser.parser->lineAt(i).lrc);
				if (lrc_parser.parser->hasTranslation()) {
					lyricText << QString::fromStdWString(lrc_parser.parser->lineAt(i).tlrc);
				}
			}
			QString lyricData = lyricText.join("\r\n"_str);

			QString durationStr = formatDuration(
				static_cast<double>(lrc_parser.parser->last().timestamp.count()) / 1000.0);

			QString karaokeStr = lrc_parser.parser->isKaraoke() ? tr("Yes") : tr("No");
			QString titleStr = lrc_parser.candidate.song;
			QString artistStr = lrc_parser.candidate.singer;
			QString albumStr = lrc_parser.candidate.albumName;

			// 新增一行
			int newRow = model_->rowCount();
			model_->insertRow(newRow);

			// 放進 QStandardItem 裡
			auto id = QString::fromLatin1(generateUuid());

			// 第 0 欄: Type (並且把 id 存進 UserRole)
			auto* typeItem = new QStandardItem(typeStr);
			typeItem->setData(id, Qt::UserRole);

			// 第 1 欄: Album
			auto* albumItem = new QStandardItem(albumStr);

			// 第 2 欄: Artist
			auto* artistItem = new QStandardItem(artistStr);

			// 第 3 欄: Title
			auto* titleItem = new QStandardItem(titleStr);

			// 第 4 欄: Lyrics
			auto* lyricItem = new QStandardItem(lyricData);

			// 第 5 欄: Karaoke
			auto* karaokeItem = new QStandardItem(karaokeStr);

			// 第 6 欄: Duration
			auto* durationItem = new QStandardItem(durationStr);

			// 將這些 QStandardItem 放入 model
			model_->setItem(newRow, LRC_PAGE_TAB_ID, typeItem);
			model_->setItem(newRow, LRC_PAGE_TAB_ALBUM, albumItem);
			model_->setItem(newRow, LRC_PAGE_TAB_ARTIST, artistItem);
			model_->setItem(newRow, LRC_PAGE_TAB_TITLE, titleItem);
			model_->setItem(newRow, LRC_PAGE_TAB_LYRICS, lyricItem);
			model_->setItem(newRow, LRC_PAGE_TAB_KARAOKE, karaokeItem);
			model_->setItem(newRow, LRC_PAGE_TAB_DURATION, durationItem);

			// id -> lrc_parser 放入 hash
			parser_map_.insert(id, lrc_parser);
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
	if (results.isEmpty()) {
		return;
	}

	change_lrc_button_->setEnabled(true);

	for (auto& result : results) {
		if (result.parsers.isEmpty()) {
			continue;
		}
		lyrics_results_.append(result);
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

void LrcPage::disableLoadLrcButton() {
	change_lrc_button_->setEnabled(false);
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

	//painter.fillRect(rect(), qTheme.backgroundColor());

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
		//lyrics_widget_->setNormalColor(Qt::lightGray);
		//lyrics_widget_->setHighLightColor(Qt::white);
		lyrics_widget_->setNormalColor(LyricsShowWidget::kNormalColor);
		lyrics_widget_->setHighLightColor(LyricsShowWidget::kHighLightColor);
		break;
	case ThemeColor::LIGHT_THEME:
		lyrics_widget_->setNormalColor(Qt::darkGray);
		lyrics_widget_->setHighLightColor(Qt::black);
		lyrics_widget_->setKaraokeHighlightColor(qTheme.highlightColor());
		break;
	}
	//setStyleSheet(qFormat("background-color: %1; border: none;").arg(
	//	colorToString(qTheme.backgroundColor())));
}

void LrcPage::initial() {
	setObjectName("lrcPage");
	setFrameShape(QFrame::StyledPanel);

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
	change_lrc_button_ = new QToolButton(this);
	change_lrc_button_->setText(tr("Load LRC"));
	change_lrc_button_->setIcon(qTheme.fontIcon(Glyphs::ICON_SUBTITLE));
	change_lrc_button_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	change_lrc_button_->setEnabled(false);
	(void)QObject::connect(change_lrc_button_, &QToolButton::clicked, [this]() {
		if (lyrics_results_.isEmpty()) {
			return;
		}
		QScopedPointer<MaskWidget> mask_widget(new MaskWidget(getMainWindow()));
		const QScopedPointer<XDialog> dialog(new XDialog(this));
		const QScopedPointer<LyricsFrame> lyrics_frame(new LyricsFrame(dialog.get()));
		dialog->setTitle(tr("Load LRC"));
		lyrics_frame->setLyrics(lyrics_results_);
		dialog->setContentWidget(lyrics_frame.get());
		dialog->setFixedSize(QSize(1000, 600));
		dialog->setIcon(qTheme.fontIcon(Glyphs::ICON_SUBTITLE));
		(void)QObject::connect(lyrics_frame.get(),
			&LyricsFrame::changeLyric,
			[this](const LyricsParser& parser) {
			if (parser.content.isEmpty()) {
				XAMP_LOG_DEBUG("Lrc content is empty!");
				return;
			}
			QString save_parent_path;
			if (!entity_.yt_music_album_id.isEmpty()) {
				save_parent_path = qAppSettings.getOrCreateLrcCachePath();
			} else {
				save_parent_path = entity_.parent_path;
			}
			auto krc_file_name = save_parent_path
				+ "\\"_str
				+ entity_.file_name.trimmed()
				+ ".krc"_str;
			QSaveFile file(krc_file_name);
			file.open(QIODevice::WriteOnly);
			file.write(parser.content);
			if (!file.commit()) {
				XAMP_LOG_DEBUG("Save lrc file failure!");
			}
			lyrics_widget_->loadFromParser(parser.parser);
			});
		dialog->exec();
		});

	change_color_button_ = new QToolButton(this);
	change_color_button_->setText(tr("Text color"));
	change_color_button_->setIcon(qTheme.fontIcon(Glyphs::ICON_SETTINGS));
	change_color_button_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	(void)QObject::connect(change_color_button_, &QToolButton::clicked, [this]() {
		QColorDialog color_dialog;
		color_dialog.setCurrentColor(lyrics_widget_->normalColor());
		(void)QObject::connect(&color_dialog, &QColorDialog::currentColorChanged, [this](auto color) {
			lyrics_widget_->setNormalColor(color);
			});
		color_dialog.exec();
		});
	change_color_button_->hide();

	change_highlight_button_ = new QToolButton(this);
	change_highlight_button_->setText(tr("Highlight text color"));
	change_highlight_button_->setIcon(qTheme.fontIcon(Glyphs::ICON_SETTINGS));
	change_highlight_button_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	(void)QObject::connect(change_highlight_button_, &QToolButton::clicked, [this]() {
		QColorDialog color_dialog;
		color_dialog.setCurrentColor(lyrics_widget_->highlightColor());
		(void)QObject::connect(&color_dialog, &QColorDialog::currentColorChanged, [this](auto color) {
			lyrics_widget_->setHighLightColor(color);
			});
		color_dialog.exec();
		});
	change_highlight_button_->hide();

	karaoke_highlight_button_ = new QToolButton(this);
	karaoke_highlight_button_->setText(tr("Karaoke highlight text color"));
	karaoke_highlight_button_->setIcon(qTheme.fontIcon(Glyphs::ICON_SETTINGS));
	karaoke_highlight_button_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	(void)QObject::connect(karaoke_highlight_button_, &QToolButton::clicked, [this]() {
		QColorDialog color_dialog;
		color_dialog.setCurrentColor(lyrics_widget_->karaokeHighlightColor());
		(void)QObject::connect(&color_dialog, &QColorDialog::currentColorChanged, [this](auto color) {
			lyrics_widget_->setKaraokeHighlightColor(color);
			});
		color_dialog.exec();
		});
	karaoke_highlight_button_->hide();

	auto horizontal_spacer_5 = new QSpacerItem(10, 20, QSizePolicy::Expanding, QSizePolicy::Fixed);
	horizontal_layout_11->addWidget(change_lrc_button_);
	horizontal_layout_11->addWidget(change_color_button_);
	horizontal_layout_11->addWidget(change_highlight_button_);
	horizontal_layout_11->addWidget(karaoke_highlight_button_);
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
	//setStyleSheet("QFrame#lrcPage{ background-color: transparent; border: none; }"_str);
	//setStyleSheet(qFormat("background-color: %1; border: none;").arg(
	//	colorToString(qTheme.backgroundColor())));
	setStyleSheet("QFrame#lrcPage{ background-color: #121212; border: none; }"_str);
}
