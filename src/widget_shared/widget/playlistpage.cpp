#include <QCompleter>
#include <widget/playlistpage.h>

#include <QGraphicsOpacityEffect>
#include <QToolButton>
#include <QVBoxLayout>
#include <QSpacerItem>
#include <QStandardItemModel>

#include <thememanager.h>
#include <widget/image_utiltis.h>
#include <widget/imagecache.h>
#include <widget/database.h>
#include <widget/str_utilts.h>
#include <widget/scrolllabel.h>
#include <widget/playlisttableview.h>

PlaylistPage::PlaylistPage(QWidget* parent)
	: QFrame(parent) {
	Initial();
}

void PlaylistPage::Initial() {
	setFrameStyle(QFrame::StyledPanel);

	auto* default_layout = new QVBoxLayout(this);
	default_layout->setSpacing(0);
	default_layout->setObjectName(QString::fromUtf8("default_layout"));
	default_layout->setContentsMargins(0, 0, 0, 0);

	auto* child_layout = new QHBoxLayout();
	child_layout->setSpacing(0);
	child_layout->setObjectName(QString::fromUtf8("horizontalLayout_7"));
	child_layout->setContentsMargins(20, 0, 0, 0);

	auto* left_space_layout = new QVBoxLayout();
	left_space_layout->setSpacing(0);
	left_space_layout->setObjectName(QString::fromUtf8("verticalLayout_3"));
	//vertical_spacer_ = new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Fixed);
	//left_space_layout->addItem(vertical_spacer_);

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
	auto f = font();
	f.setWeight(QFont::DemiBold);
	f.setPointSize(qTheme.GetFontSize(20));

	const QFontMetrics metrics(f);
	const auto font_height = metrics.height();

	title_->setFont(f);
	title_->setObjectName(QString::fromUtf8("label_2"));
	title_->setMinimumSize(QSize(0, font_height));
	title_->setMaximumSize(QSize(16777215, font_height));
	title_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	heart_button_ = new QToolButton(this);
	heart_button_->setObjectName(QString::fromUtf8("heartButton"));
	heart_button_->setMinimumSize(QSize(24, 24));
	heart_button_->setMaximumSize(QSize(24, 24));
	heart_button_->setIconSize(QSize(24, 24));
	qTheme.SetHeartButton(heart_button_);
	heart_button_->hide();

	(void)QObject::connect(heart_button_, &QToolButton::clicked, [this]() {
		if (album_id_) {
			album_heart_ = !album_heart_;
			qMainDb.UpdateAlbumHeart(album_id_.value(), album_heart_);
			qTheme.SetHeartButton(heart_button_, album_heart_);
		}
		});

	search_line_edit_ = new QLineEdit();
	search_line_edit_->setObjectName(QString::fromUtf8("playlistSearchLineEdit"));
	QSizePolicy size_policy3(QSizePolicy::Fixed, QSizePolicy::Fixed);
	size_policy3.setHorizontalStretch(0);
	size_policy3.setVerticalStretch(0);
	size_policy3.setHeightForWidth(search_line_edit_->sizePolicy().hasHeightForWidth());
	search_line_edit_->setSizePolicy(size_policy3);
	search_line_edit_->setMinimumSize(QSize(180, 30));
	search_line_edit_->setFocusPolicy(Qt::ClickFocus);
	search_line_edit_->setClearButtonEnabled(true);
	search_line_edit_->addAction(qTheme.GetFontIcon(Glyphs::ICON_SEARCH), QLineEdit::TrailingPosition);
	search_line_edit_->setPlaceholderText(tr("Search Album/Title"));

	format_ = new QLabel(this);
	const QFont format_font(qTEXT("FormatFont"));
	format_->setFont(format_font);

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

	default_layout->addLayout(child_layout);

	default_spacer_ = new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Fixed);

	default_layout->addItem(default_spacer_);

	auto* horizontalLayout_8 = new QHBoxLayout();
	horizontalLayout_8->setSpacing(0);
	horizontalLayout_8->setObjectName(QString::fromUtf8("horizontalLayout_8"));
	horizontal_spacer_4_ = new QSpacerItem(5, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

	horizontalLayout_8->addItem(horizontal_spacer_4_);

	playlist_ = new PlayListTableView(this);
	playlist_->setObjectName(QString::fromUtf8("tableView"));

	horizontalLayout_8->addWidget(playlist_);
	horizontal_spacer_5_ = new QSpacerItem(5, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
	horizontalLayout_8->addItem(horizontal_spacer_5_);
	horizontalLayout_8->setStretch(1, 1);

	auto* horizontal_layout_9 = new QHBoxLayout();
	horizontal_layout_9->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding));
	horizontal_layout_9->addWidget(search_line_edit_, 1);
	horizontal_layout_9->setContentsMargins(0, 0, 8, 10);
	default_layout->addLayout(horizontal_layout_9);

	default_layout->addLayout(horizontalLayout_8);
	default_layout->setStretch(3, 1);

	(void)QObject::connect(playlist_,
		&PlayListTableView::UpdateAlbumCover,
		this,
		&PlaylistPage::SetCoverById);

	auto * search_playlist_model = new QStandardItemModel(0, 1, this);
	auto* playlist_completer = new QCompleter(search_playlist_model, this);
	playlist_completer->setCaseSensitivity(Qt::CaseInsensitive);
	playlist_completer->setFilterMode(Qt::MatchContains);
	playlist_completer->setCompletionMode(QCompleter::PopupCompletion);
	search_line_edit_->setCompleter(playlist_completer);

	auto action_list = search_line_edit_->findChildren<QAction*>();
	if (!action_list.isEmpty()) {
		(void)QObject::connect(action_list.first(), &QAction::triggered, this, [this]() {
			playlist_->Search(kEmptyString);
			});
	}

	(void)QObject::connect(search_line_edit_, &QLineEdit::textChanged, [this, search_playlist_model, playlist_completer](const auto& text) {
		const auto items = search_playlist_model->findItems(text, Qt::MatchExactly);
		if (!items.isEmpty()) {
			playlist_->Search(kEmptyString);
			return;
		}
		if (search_playlist_model->rowCount() >= kMaxCompletionCount) {
			search_playlist_model->removeRows(0, kMaxCompletionCount - search_playlist_model->rowCount() + 1);
		}

		search_playlist_model->appendRow(new QStandardItem(text));
		playlist_completer->setModel(search_playlist_model);
		playlist_completer->setCompletionPrefix(text);

		playlist_->Search(text);
		});
	qTheme.SetLineEditStyle(search_line_edit_, qTEXT("playlistSearchLineEdit"));
}

void PlaylistPage::OnThemeColorChanged(QColor theme_color, QColor color) {
	title_->setStyleSheet(qTEXT("QLabel { color: ") + ColorToString(color) + qTEXT("; background-color: transparent; }"));
	format_->setStyleSheet(qTEXT("QLabel { font-family: FormatFont; font-size: 16px; color: ") + ColorToString(color) + qTEXT("; background-color: transparent; }"));
}

void PlaylistPage::SetHeart(bool heart) {
	heart_button_->show();
	qTheme.SetHeartButton(heart_button_, heart);
}

QLabel* PlaylistPage::format() {
	return format_;
}

ScrollLabel* PlaylistPage::title() {
	return title_;
}

QLabel* PlaylistPage::cover() {
	return cover_;
}

void PlaylistPage::HidePlaybackInformation(bool hide) {
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

void PlaylistPage::SetAlbumId(int32_t album_id, int32_t heart) {
	album_id_ = album_id;
	album_heart_ = heart;
	qTheme.SetHeartButton(heart_button_, album_heart_);
}

void PlaylistPage::SetCover(const QPixmap * cover) {
	const auto playlist_cover = image_utils::RoundImage(
		image_utils::ResizeImage(*cover, QSize(165, 165), false),
		image_utils::kPlaylistImageRadius);
	cover_->setPixmap(playlist_cover);
}

void PlaylistPage::SetCoverById(const QString& cover_id) {
	const auto cover = qImageCache.GetOrDefault(cover_id);
	SetCover(&cover);
}

PlayListTableView* PlaylistPage::playlist() {
    return playlist_;
}
