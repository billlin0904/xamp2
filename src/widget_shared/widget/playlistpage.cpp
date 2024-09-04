#include <QCompleter>
#include <widget/playlistpage.h>

#include <QGraphicsOpacityEffect>
#include <QToolButton>
#include <QVBoxLayout>
#include <QSpacerItem>
#include <QStandardItemModel>

#include <thememanager.h>
#include <widget/util/image_util.h>
#include <widget/imagecache.h>
#include <widget/dao/albumdao.h>
#include <widget/util/str_util.h>
#include <widget/scrolllabel.h>
#include <widget/processindicator.h>
#include <widget/playlisttableview.h>

PlaylistPage::PlaylistPage(QWidget* parent)
	: QFrame(parent) {
	setObjectName("playlistPage");
	setFrameShape(QFrame::StyledPanel);
	initial();
    setStyleSheet(qTEXT("background-color: transparent;"));
}

ProcessIndicator* PlaylistPage::spinner() {
	return spinner_;
}

void PlaylistPage::initial() {
	spinner_ = new ProcessIndicator(this);
	spinner_->hide();

	auto* main_layout = new QVBoxLayout(this);
	main_layout->setSpacing(0);
	main_layout->setObjectName(QString::fromUtf8("default_layout"));
	main_layout->setContentsMargins(0, 0, 0, 0);

	auto f = font();
	page_title_label_ = new QLabel(tr("Playlist"), this);
	page_title_label_->setStyleSheet(qTEXT("background-color: transparent;"));
	f.setBold(true);
	f.setPointSize(qTheme.fontSize(36));
	page_title_label_->setFont(f);
	main_layout->addWidget(page_title_label_);

	auto* child_layout = new QHBoxLayout();
	child_layout->setSpacing(0);
	child_layout->setObjectName(QString::fromUtf8("horizontalLayout_7"));
	child_layout->setContentsMargins(20, 0, 0, 0);

	auto* left_space_layout = new QVBoxLayout();
	left_space_layout->setSpacing(0);
	left_space_layout->setObjectName(QString::fromUtf8("verticalLayout_3"));
	vertical_spacer_ = new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Fixed);
	left_space_layout->addItem(vertical_spacer_);

	cover_ = new QLabel();
	cover_->setObjectName(QString::fromUtf8("label"));
	cover_->setMinimumSize(QSize(200, 200));
	cover_->setMaximumSize(QSize(200, 200));
	cover_->setAttribute(Qt::WA_StaticContents);

	left_space_layout->addWidget(cover_);

	child_layout->addLayout(left_space_layout, 0);

	horizontal_spacer_ = new QSpacerItem(40, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

	child_layout->setStretch(0, 1);
	child_layout->setStretch(1, 2);
	child_layout->addItem(horizontal_spacer_);

	auto* album_title_layout = new QVBoxLayout();
	album_title_layout->setSpacing(0);
	album_title_layout->setObjectName(QString::fromUtf8("verticalLayout_2"));
	album_title_layout->setContentsMargins(0, 5, -1, -1);

    title_ = new ScrollLabel(this);
	f = font();
	f.setWeight(QFont::DemiBold);
	f.setPointSize(qTheme.fontSize(15));

	const QFontMetrics metrics(f);
	const auto font_height = metrics.height() * 1.2;

	title_->setFont(f);
	title_->setObjectName(QString::fromUtf8("label_2"));
	title_->setMinimumSize(QSize(0, font_height));
	title_->setMaximumSize(QSize(16777215, font_height));
	title_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	title_->setText(qTEXT("\0")); // 預先配置空字串保留行高
	title_->setStyleSheet(qTEXT("background-color: transparent;"));

	heart_button_ = new QToolButton(this);
	heart_button_->setObjectName(QString::fromUtf8("heartButton"));
	heart_button_->setMinimumSize(QSize(24, 24));
	heart_button_->setMaximumSize(QSize(24, 24));
	heart_button_->setIconSize(QSize(24, 24));
	qTheme.setHeartButton(heart_button_);

	(void)QObject::connect(heart_button_, &QToolButton::clicked, [this]() {
		if (album_id_) {
			album_heart_ = !album_heart_;
			dao::AlbumDao(qGuiDb.getDatabase()).updateAlbumHeart(album_id_.value(), album_heart_);
			qTheme.setHeartButton(heart_button_, album_heart_);
		}
		});

    f.setPointSize(qTheme.fontSize(9));
	search_line_edit_ = new QLineEdit();
	search_line_edit_->setObjectName(QString::fromUtf8("playlistSearchLineEdit"));
	QSizePolicy size_policy3(QSizePolicy::Fixed, QSizePolicy::Fixed);
	size_policy3.setHorizontalStretch(0);
	size_policy3.setVerticalStretch(0);
	size_policy3.setHeightForWidth(search_line_edit_->sizePolicy().hasHeightForWidth());
	search_line_edit_->setSizePolicy(size_policy3);
	search_line_edit_->setMinimumSize(QSize(250, 30));
	search_line_edit_->setFocusPolicy(Qt::ClickFocus);
	search_line_edit_->setClearButtonEnabled(true);
    search_line_edit_->setFont(f);
	search_line_action_ = search_line_edit_->addAction(qTheme.fontIcon(Glyphs::ICON_SEARCH), QLineEdit::TrailingPosition);
	search_line_edit_->setPlaceholderText(tr("Search Album/Artist/Title"));

	format_ = new QLabel(this);
	const QFont format_font(qTEXT("FormatFont"));
	format_->setFont(format_font);
	format_->setText(qTEXT("\0"));
	format_->setStyleSheet(qTEXT("background-color: transparent;"));

	vertical_spacer_ = new QSpacerItem(20, 24, QSizePolicy::Minimum, QSizePolicy::Fixed);
	album_title_layout->addItem(vertical_spacer_);
	album_title_layout->addWidget(title_);

	middle_spacer_ = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);
	album_title_layout->addItem(middle_spacer_);	
	album_title_layout->addWidget(format_);
	album_title_layout->addWidget(heart_button_);

	right_spacer_ = new QSpacerItem(20, 108, QSizePolicy::Minimum, QSizePolicy::Expanding);
	album_title_layout->addItem(right_spacer_);	
	album_title_layout->setStretch(0, 1);

	child_layout->addLayout(album_title_layout, 0);
	child_layout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));
	album_title_layout->setContentsMargins(0, 10, 0, 0);

	main_layout->addLayout(child_layout);

	default_spacer_ = new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Fixed);
	main_layout->addItem(default_spacer_);

	auto* horizontalLayout_8 = new QHBoxLayout();
	horizontalLayout_8->setSpacing(0);
	horizontalLayout_8->setObjectName(QString::fromUtf8("horizontalLayout_8"));
	horizontalLayout_8->setContentsMargins(0, 0, 0, 0);
	horizontal_spacer_4_ = new QSpacerItem(5, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

	horizontalLayout_8->addItem(horizontal_spacer_4_);

	playlist_ = new PlaylistTableView(this);
	playlist_->setObjectName(QString::fromUtf8("tableView"));
	playlist_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	horizontalLayout_8->addWidget(playlist_, 1);

	horizontal_spacer_5_ = new QSpacerItem(5, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
	horizontalLayout_8->addItem(horizontal_spacer_5_);
	horizontalLayout_8->setStretch(1, 1);

	auto* horizontal_layout_9 = new QHBoxLayout();
	horizontal_layout_9->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding));
	horizontal_layout_9->addWidget(search_line_edit_, 1);
	horizontal_layout_9->setContentsMargins(0, 0, 8, 10);
	main_layout->addLayout(horizontal_layout_9);

	main_layout->addLayout(horizontalLayout_8);
	main_layout->setStretch(4, 1);
	//main_layout->addWidget(playlist_);

	(void)QObject::connect(playlist_,
		&PlaylistTableView::updateAlbumCover,
		this,
		&PlaylistPage::onSetCoverById);

	search_playlist_model_ = new QStandardItemModel(0, 1, this);
	playlist_completer_ = new QCompleter(search_playlist_model_, this);
	playlist_completer_->setCaseSensitivity(Qt::CaseInsensitive);
	playlist_completer_->setFilterMode(Qt::MatchContains);
	playlist_completer_->setCompletionMode(QCompleter::PopupCompletion);
	search_line_edit_->setCompleter(playlist_completer_);

	auto action_list = search_line_edit_->findChildren<QAction*>();
	if (!action_list.isEmpty()) {
		(void)QObject::connect(action_list.first(), &QAction::triggered, this, [this]() {
			emit search(kEmptyString, MATCH_NONE);
			});
	}

	(void)QObject::connect(playlist_completer_, static_cast<void (QCompleter::*)(const QString &)>(&QCompleter::activated), [this](const auto & text) {
		emit editFinished(text);
		});

	(void)QObject::connect(search_line_edit_, &QLineEdit::returnPressed, [this]() {
		const auto text = search_line_edit_->text();
		emit search(text, MATCH_SUGGEST);
		});

	(void)QObject::connect(search_line_edit_, &QLineEdit::textChanged, [this](const auto& text) {
		const auto items = search_playlist_model_->findItems(text, Qt::MatchExactly);
		if (!items.isEmpty()) {
			emit search(text, MATCH_NONE);
			return;
		}
		if (search_playlist_model_->rowCount() >= kMaxCompletionCount) {
			search_playlist_model_->removeRows(0, kMaxCompletionCount - search_playlist_model_->rowCount() + 1);
		}
		search_playlist_model_->appendRow(new QStandardItem(text));
		playlist_completer_->setModel(search_playlist_model_);
		playlist_completer_->setCompletionPrefix(text);
		emit search(text, MATCH_NONE);
		});

	qTheme.setLineEditStyle(search_line_edit_, qTEXT("playlistSearchLineEdit"));	
}

void PlaylistPage::setHeart(bool heart) {
	if (!heart_button_->isHidden()) {
		heart_button_->show();
		qTheme.setHeartButton(heart_button_, heart);
	}
}

QLabel* PlaylistPage::format() {
	return format_;
}

QToolButton* PlaylistPage::heart() {
	return heart_button_;
}

ScrollLabel* PlaylistPage::title() {
	return title_;
}

QLabel* PlaylistPage::pageTitle() {
	return page_title_label_;
}

QLabel* PlaylistPage::cover() {
	return cover_;
}

void PlaylistPage::hidePlaybackInformation(bool hide) {
	if (hide) {
		format_->hide();
		title_->hide();
		cover_->hide();
		heart_button_->hide();
		vertical_spacer_->changeSize(0, 0);
		horizontal_spacer_->changeSize(0, 0);
		middle_spacer_->changeSize(0, 0);
		right_spacer_->changeSize(0, 0);
		default_spacer_->changeSize(0, 0);
		horizontal_spacer_4_->changeSize(0, 0);
		horizontal_spacer_5_->changeSize(0, 0);
	}
}

void PlaylistPage::setAlbumId(int32_t album_id, int32_t heart) {
	album_id_ = album_id;
	album_heart_ = heart;
	if (!heart_button_->isHidden()) {
		qTheme.setHeartButton(heart_button_, album_heart_);
	}
}

void PlaylistPage::addSuggestions(const QString& text) {
	search_playlist_model_->appendRow(new QStandardItem(text));
}

void PlaylistPage::showCompleter() {
	search_line_edit_->completer()->complete();
}

void PlaylistPage::setCover(const QPixmap * cover) {
	auto cover_size = cover_->size();
	cover_size = QSize(cover_size.width() - image_util::kPlaylistImageRadius,
		cover_size.height() - image_util::kPlaylistImageRadius);
	const auto playlist_cover = image_util::roundImage(
		image_util::resizeImage(*cover, cover_size, false),
		image_util::kPlaylistImageRadius);
	cover_->setPixmap(playlist_cover);
}

void PlaylistPage::onThemeChangedFinished(ThemeColor theme_color) {
	title_->setStyleSheet(qTEXT("QLabel { color: ") + colorToString(qTheme.textColor()) + qTEXT("; background-color: transparent; }"));
	format_->setStyleSheet(qTEXT("QLabel { font-family: FormatFont; font-size: 16px; color: ") + colorToString(qTheme.textColor()) + qTEXT("; background-color: transparent; }"));
	qTheme.setLineEditStyle(search_line_edit_, qTEXT("playlistSearchLineEdit"));
	search_line_action_->setIcon(qTheme.fontIcon(Glyphs::ICON_SEARCH));	
}

void PlaylistPage::onSetCoverById(const QString& cover_id) {
	const auto cover = qImageCache.getOrAddDefault(cover_id);
	setCover(&cover);
}

void PlaylistPage::onRetranslateUi() {
}

PlaylistTableView* PlaylistPage::playlist() {
    return playlist_;
}
