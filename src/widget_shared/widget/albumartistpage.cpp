#include <widget/albumartistpage.h>

#include <QVBoxLayout>
#include <QStandardItemModel>
#include <QGraphicsOpacityEffect>
#include <QPainter>
#include <QApplication>
#include <QScrollBar>
#include <QPainterPath>
#include <QMouseEvent>
#include <QLineEdit>
#include <QComboBox>
#include <QCompleter>
#include <QScrollArea>

#include <widget/util/ui_util.h>
#include <widget/artistview.h>
#include <widget/genre_view.h>
#include <widget/clickablelabel.h>
#include <widget/util/str_util.h>
#include <widget/albumview.h>
#include <widget/artistinfopage.h>
#include <widget/dao/albumdao.h>
#include <widget/taglistview.h>
#include <widget/genre_view_page.h>

#include <xampplayer.h>
#include <thememanager.h>

enum TAB_INDEX : std::uint8_t {
	TAB_ALBUMS,
	TAB_ARTISTS,
	TAB_YEAR,
	//TAB_SIMILAR_SONG
};

AlbumTabListView::AlbumTabListView(QWidget* parent)
	: QListView(parent)
	, model_(this) {
	setModel(&model_);
	setFrameStyle(QFrame::StyledPanel);
	setEditTriggers(QAbstractItemView::NoEditTriggers);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setHorizontalScrollMode(QListView::ScrollPerPixel);
	setSpacing(2);
	setIconSize(qTheme.tabIconSize());
	setFlow(QListView::Flow::LeftToRight);

	(void)QObject::connect(this, &QListView::clicked, [this](auto index) {
		auto table_id = index.data(Qt::UserRole + 1).toInt();
		emit clickedTable(table_id);
		});

	(void)QObject::connect(&model_, &QStandardItemModel::itemChanged, [this](auto item) {
		auto table_id = item->data(Qt::UserRole + 1).toInt();
		auto table_name = item->data(Qt::DisplayRole).toString();
		emit tableNameChanged(table_id, table_name);
		});
}

void AlbumTabListView::addTab(const QString& name, int tab_id) {
	auto* item = new QStandardItem(name);
	item->setData(tab_id);
    item->setSizeHint(QSize(200, 30));
	item->setTextAlignment(Qt::AlignCenter);
	auto f = item->font();
#ifdef Q_OS_WIN
    f.setPointSize(qTheme.fontSize(20));
#else
    f.setPointSize(qTheme.fontSize(14));
#endif
	f.setBold(true);
	item->setFont(f);
	model_.appendRow(item);
}

void AlbumTabListView::setCurrentTab(int tab_id) {
	setCurrentIndex(model_.index(tab_id, 0));
}

void AlbumTabListView::setTabText(const QString& name, int tab_id) {
	auto* item = model_.item(tab_id);
	if (item) {
		item->setText(name);
	}
}

AlbumArtistPage::AlbumArtistPage(QWidget* parent)
	: QFrame(parent) {
	//, album_tab_list_view_(new AlbumTabListView(this))
	//, album_view_(new AlbumView(this))
	//, recent_plays_album_view_(new AlbumView(this))
	//, artist_view_(new ArtistView(this))
	//, artist_info_view_(new ArtistInfoPage(this)) {
	//album_view_->reload();
	//artist_view_->reload();

	auto* vertical_layout_2 = new QVBoxLayout(this);
	vertical_layout_2->setSpacing(0);
	vertical_layout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
	vertical_layout_2->setContentsMargins(11, 0, 11, 0);

	auto f = font();
	page_title_label_ = new QLabel(tr("Library"), this);
	f.setBold(true);
	f.setPointSize(qTheme.fontSize(40));
	page_title_label_->setFont(f);
	vertical_layout_2->addWidget(page_title_label_);

	auto* line = new QFrame();
	line->setFrameShape(QFrame::HLine);
	vertical_layout_2->addWidget(line, 1);

	auto* horizontal_layout_5 = new QHBoxLayout();
	horizontal_layout_5->setSpacing(6);
	horizontal_layout_5->setObjectName(QString::fromUtf8("horizontalLayout_5"));

	auto* horizontal_spacer_6 = new QSpacerItem(50, 50, QSizePolicy::Expanding, QSizePolicy::Expanding);
	horizontal_layout_5->addItem(horizontal_spacer_6);

	album_tab_list_view_ = new AlbumTabListView(this);
	album_tab_list_view_->setObjectName(QString::fromUtf8("albumTab"));
	album_tab_list_view_->addTab(tr("ALBUMS"), TAB_ALBUMS);
	album_tab_list_view_->addTab(tr("ARTISTS"), TAB_ARTISTS);
	album_tab_list_view_->addTab(tr("YEAR"), TAB_YEAR);
	//album_tab_list_view_->addTab(tr("SIMILAR"), TAB_SIMILAR_SONG);

	qTheme.setAlbumNaviBarTheme(album_tab_list_view_);

	constexpr QSizePolicy size_policy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	album_tab_list_view_->setSizePolicy(size_policy);
	album_tab_list_view_->setMinimumSize(830, 40);
	album_tab_list_view_->setMaximumHeight(40);
	horizontal_layout_5->addWidget(album_tab_list_view_);

	auto* horizontal_spacer_7 = new QSpacerItem(50, 50, QSizePolicy::Expanding, QSizePolicy::Expanding);
	horizontal_layout_5->addItem(horizontal_spacer_7);

	auto* current_view = new QStackedWidget(this);
	current_view->setObjectName(QString::fromUtf8("currentView"));
	current_view->setFrameShadow(QFrame::Plain);
	current_view->setLineWidth(0);

	album_frame_ = new QFrame(this);
	album_frame_->setObjectName(QString::fromUtf8("currentAlbumViewFrame"));
	album_frame_->setFrameShape(QFrame::StyledPanel);

	auto* album_frame_layout = new QVBoxLayout(album_frame_);
	album_frame_layout->setSpacing(0);
	album_frame_layout->setObjectName(QString::fromUtf8("currentAlbumViewFrameLayout"));
	album_frame_layout->setContentsMargins(0, 0, 0, 0);

	auto* album_combox_layout_1 = new QHBoxLayout();
	auto* album_combox_layout = new QHBoxLayout();

	f.setBold(true);
	f.setPointSize(qTheme.fontSize(10));

	auto title_category_list = dao::AlbumDao(qGuiDb.getDatabase()).getCategories();

	album_tag_list_widget_ = new TagListView();
	album_tag_list_widget_->setListViewFixedHeight(70);

    std::sort(title_category_list.begin(), title_category_list.end(),
              [](const auto& left, const auto& right) {
		return left.length() < right.length();
		});
	
	Q_FOREACH(auto category, title_category_list) {
		album_tag_list_widget_->addTag(category);
	}
	album_tag_list_widget_->addTag(tr("All"), true);

	(void)QObject::connect(album_tag_list_widget_, &TagListView::tagChanged, [this](const auto& tags) {
		if (tags.isEmpty()) {
			return;
		}
		if (tags.contains(tr("All"))) {
			album_view_->showAll();
			album_tag_list_widget_->disableAllTag(tr("All"));
		}
		else {
			album_view_->filterCategories(tags, AlbumView::FILTER_AND);
		}
		album_view_->albumViewPage()->hide();
		album_view_->reload();
		});

	(void)QObject::connect(album_tag_list_widget_, &TagListView::tagClear, [this]() {
		album_view_->showAll();
		album_view_->reload();
		});

    f.setPointSize(qTheme.fontSize(9));

	album_search_line_edit_ = new QLineEdit();
    album_search_line_edit_->setFont(f);
	album_search_line_edit_->setObjectName(QString::fromUtf8("albumSearchLineEdit"));
	QSizePolicy size_policy3(QSizePolicy::Fixed, QSizePolicy::Fixed);
	size_policy3.setHorizontalStretch(0);
	size_policy3.setVerticalStretch(0);
	size_policy3.setHeightForWidth(album_search_line_edit_->sizePolicy().hasHeightForWidth());
	album_search_line_edit_->setSizePolicy(size_policy3);
	album_search_line_edit_->setMinimumSize(QSize(180, 30));
	album_search_line_edit_->setFocusPolicy(Qt::ClickFocus);
	album_search_line_edit_->setClearButtonEnabled(true);
	album_search_action_ = album_search_line_edit_->addAction(qTheme.fontIcon(Glyphs::ICON_SEARCH), QLineEdit::TrailingPosition);
	album_search_line_edit_->setPlaceholderText(tr("Search Album/Artist"));
	
	album_model_ = new QStandardItemModel(0, 1 , this);
	auto *album_completer = new QCompleter(album_model_, this);
	album_completer->setCaseSensitivity(Qt::CaseInsensitive);
	album_completer->setFilterMode(Qt::MatchContains);
	album_completer->setCompletionMode(QCompleter::PopupCompletion);
	album_search_line_edit_->setCompleter(album_completer);

	auto action_list = album_search_line_edit_->findChildren<QAction*>();
	if (!action_list.isEmpty()) {
		Q_FOREACH(auto * action, action_list) {
			if (action) {
				(void)QObject::connect(action, &QAction::triggered, [this]() {
					album_view_->search(kEmptyString);
					album_view_->reload();
					});
			}
		}
	}

	(void)QObject::connect(album_search_line_edit_, &QLineEdit::textChanged, [this, album_completer](const auto& text) {
		const auto items = album_model_->findItems(text, Qt::MatchExactly);
		if (!items.isEmpty()) {
			album_view_->search(kEmptyString);
			return;
		}
		if (album_model_->rowCount() >= kMaxCompletionCount) {
			album_model_->removeRows(0, kMaxCompletionCount - album_model_->rowCount() + 1);
		}

		album_model_->appendRow(new QStandardItem(text));
		album_completer->setModel(album_model_);
		album_completer->setCompletionPrefix(text);

		album_view_->search(text);
		});

	album_combox_layout->addWidget(album_search_line_edit_);	

	auto horizontal_spacer = new QSpacerItem(20, 50, QSizePolicy::Expanding, QSizePolicy::Expanding);
	album_combox_layout_1->addSpacerItem(horizontal_spacer);

	album_combox_layout_1->addLayout(album_combox_layout);
	album_frame_layout->addLayout(album_combox_layout_1);

	// Recent Plays

	/*auto* recent_plays_tags_label = new QLabel(tr("Recent Plays"));
	f.setPointSize(qTheme.fontSize(12));
	recent_plays_tags_label->setFont(f);

	album_frame_layout->addWidget(recent_plays_tags_label);
	recent_plays_album_view_->setFixedHeight(280);
	recent_plays_album_view_->filterRecentPlays();
	recent_plays_album_view_->reload();
	album_frame_layout->addWidget(recent_plays_album_view_, 1);*/

	// Category

	auto* tags_label = new QLabel(tr("Category"));
	f.setPointSize(qTheme.fontSize(12));
	tags_label->setFont(f);

	album_frame_layout->addWidget(tags_label);
	album_frame_layout->addWidget(album_tag_list_widget_);	 
	album_view_ = new AlbumView(this);
	album_frame_layout->addWidget(album_view_, 1);
	
	artist_frame_ = new QFrame();
	artist_frame_->setObjectName(QString::fromUtf8("currentArtistViewFrame"));
	artist_frame_->setFrameShape(QFrame::StyledPanel);
	auto* artist_frame_layout = new QVBoxLayout(artist_frame_);
	artist_frame_layout->setSpacing(0);
	artist_frame_layout->setObjectName(QString::fromUtf8("currentArtistViewFrameLayout"));
	artist_frame_layout->setContentsMargins(0, 0, 0, 0);

	artist_search_line_edit_ = new QLineEdit();
    f.setPointSize(qTheme.fontSize(9));
    artist_search_line_edit_->setFont(f);
	artist_search_line_edit_->setObjectName(QString::fromUtf8("artistSearchLineEdit"));
	QSizePolicy size_policy4(QSizePolicy::Fixed, QSizePolicy::Fixed);
	size_policy3.setHorizontalStretch(0);
	size_policy3.setVerticalStretch(0);
	size_policy3.setHeightForWidth(artist_search_line_edit_->sizePolicy().hasHeightForWidth());
	artist_search_line_edit_->setSizePolicy(size_policy4);
	artist_search_line_edit_->setMinimumSize(QSize(180, 30));
	artist_search_line_edit_->setFocusPolicy(Qt::ClickFocus);
	artist_search_line_edit_->setClearButtonEnabled(true);
	artist_search_action_ = artist_search_line_edit_->addAction(qTheme.fontIcon(Glyphs::ICON_SEARCH), QLineEdit::TrailingPosition);
	artist_search_line_edit_->setPlaceholderText(tr("Search Artist"));

	action_list = artist_search_line_edit_->findChildren<QAction*>();
	if (!action_list.isEmpty()) {
		Q_FOREACH(auto * action, action_list) {
			if (action) {
				(void)QObject::connect(action, &QAction::triggered, [this]() {
					artist_view_->search(kEmptyString);
					artist_view_->reload();
					});
			}
		}
	}
	
	artist_model_ = new QStandardItemModel(0, 1, this);
	auto* artist_completer = new QCompleter(artist_model_, this);
	artist_completer->setCaseSensitivity(Qt::CaseInsensitive);
	artist_completer->setFilterMode(Qt::MatchContains);
	artist_search_line_edit_->setCompleter(artist_completer);	

	(void)QObject::connect(artist_search_line_edit_, &QLineEdit::textChanged, [artist_completer, this](const auto& text) {
		const auto items = artist_model_->findItems(text, Qt::MatchExactly);
		if (!items.isEmpty()) {
			artist_view_->search(kEmptyString);
			return;
		}
		if (artist_model_->rowCount() >= kMaxCompletionCount) {
			artist_model_->removeRows(0, kMaxCompletionCount - artist_model_->rowCount() + 1);
		}

		artist_model_->appendRow(new QStandardItem(text));
		artist_completer->setModel(artist_model_);
		artist_completer->setCompletionPrefix(text);

		artist_view_->search(text);
		artist_view_->reload();
		});

	auto horizontal_spacer_1 = new QSpacerItem(20, 50, QSizePolicy::Expanding, QSizePolicy::Expanding);
	auto* artist_combox_layout_1 = new QHBoxLayout();
	auto* artist_combox_layout = new QHBoxLayout();
	artist_combox_layout->addWidget(artist_search_line_edit_);
	artist_combox_layout_1->addSpacerItem(horizontal_spacer_1);
	artist_combox_layout_1->addLayout(artist_combox_layout);

	QStringList artist_category_list;
	for (auto i = 0; i < 26; ++i) {
		artist_category_list.append(QString(QChar('A' + i)));
	}

	artist_tag_list_widget_ = new TagListView();

	Q_FOREACH(auto category, artist_category_list) {
		artist_tag_list_widget_->addTag(category, true);
	}

	artist_frame_layout->addLayout(artist_combox_layout_1);

	artist_frame_layout->addWidget(artist_tag_list_widget_);

	artist_view_ = new ArtistView(this);
	artist_frame_layout->addWidget(artist_view_, 1);

	current_view->addWidget(album_frame_);
	current_view->addWidget(artist_frame_);

	auto* default_layout = new QHBoxLayout();
	default_layout->setSpacing(0);
	default_layout->setObjectName(QString::fromUtf8("default_layout"));
	default_layout->setContentsMargins(0, 0, 0, 0);
	default_layout->addWidget(current_view);

	vertical_layout_2->addLayout(horizontal_layout_5);
	vertical_layout_2->addLayout(default_layout, 1);

	(void)QObject::connect(artist_tag_list_widget_, &TagListView::tagChanged, [this](const auto& tags) {
		if (tags.isEmpty()) {
			return;
		}
		if (tags.contains(tr("All"))) {
			artist_view_->showAll();
			artist_tag_list_widget_->disableAllTag(tr("All"));
		}
		else {
			artist_view_->filterArtistName(tags);
		}
		artist_view_->reload();
		});

	(void)QObject::connect(artist_tag_list_widget_, &TagListView::tagClear, [this]() {
		artist_view_->showAll();
		artist_view_->reload();
		});

	year_frame_ = new QFrame();
	year_frame_->setObjectName(QString::fromUtf8("currentArtistViewFrame"));
	year_frame_->setFrameShape(QFrame::StyledPanel);
	auto* year_frame_layout = new QVBoxLayout(year_frame_);
	year_frame_layout->setSpacing(0);
	QSizePolicy size_policy_1(QSizePolicy::Preferred, QSizePolicy::Fixed);
	year_frame_layout->setObjectName(QString::fromUtf8("currentArtistViewFrameLayout"));
	year_frame_layout->setContentsMargins(0, 0, 0, 0);	

	year_view_ = new AlbumView();
	year_tag_list_widget_ = new TagListView();
	year_tag_list_widget_->setSizePolicy(size_policy_1);
	Q_FOREACH (auto year, dao::AlbumDao(qGuiDb.getDatabase()).getYears()) {
		year_tag_list_widget_->addTag(year, true);
	}
	year_frame_layout->addWidget(year_tag_list_widget_);
	year_frame_layout->addWidget(year_view_, 1);

	(void)QObject::connect(year_tag_list_widget_, &TagListView::tagChanged, [this](const QSet<QString>& tags) {
		if (tags.isEmpty()) {
			return;
		}
		if (tags.contains(tr("All"))) {
			year_view_->showAll();
			year_tag_list_widget_->disableAllTag(tr("All"));
		} else {
			year_view_->filterYears(tags);
		}
		year_view_->reload();
		album_view_->hideWidget();
		year_view_->hideWidget();
		});

	(void)QObject::connect(year_tag_list_widget_, &TagListView::tagClear, [this]() {
		year_view_->showAll();
		year_view_->reload();
		});

	current_view->addWidget(year_frame_);

	(void)QObject::connect(album_tab_list_view_, &AlbumTabListView::clickedTable, [this, current_view](auto table_id) {
		switch (table_id) {
			case TAB_ALBUMS:
				current_view->setCurrentWidget(album_frame_);
				break;
			case TAB_ARTISTS:
				current_view->setCurrentWidget(artist_frame_);
				break;
			case TAB_YEAR:
				current_view->setCurrentWidget(year_frame_);
				break;
			/*case TAB_SIMILAR_SONG:
				current_view->setCurrentWidget(similar_song_frame_);
				break;*/
			default: 
				break;
		}
		});

	current_view->setCurrentIndex(0);
	album_tab_list_view_->setCurrentTab(TAB_ALBUMS);

	(void)QObject::connect(album_view_, &AlbumView::removeAll, [this]() {
		album_tag_list_widget_->clearTag();
		year_tag_list_widget_->clearTag();
		
		artist_view_->reload();
		year_view_->reload();
		reload();
		emit removeAll();
		});

	onThemeChangedFinished(qTheme.themeColor());

	album_view_->reload();
	artist_view_->reload();

	setStyleSheet("background-color: transparent; border: none;"_str);
}

void AlbumArtistPage::onThemeChangedFinished(ThemeColor theme_color) {
	album_view_->onThemeChangedFinished(theme_color);
	artist_view_->onThemeChangedFinished(theme_color);
	year_view_->onThemeChangedFinished(theme_color);

	album_tag_list_widget_->onThemeChangedFinished(theme_color);
	artist_tag_list_widget_->onThemeChangedFinished(theme_color);

	qTheme.setLineEditStyle(album_search_line_edit_, "albumSearchLineEdit"_str);
	qTheme.setLineEditStyle(artist_search_line_edit_, "artistSearchLineEdit"_str);

	album_search_action_->setIcon(qTheme.fontIcon(Glyphs::ICON_SEARCH));
	artist_search_action_->setIcon(qTheme.fontIcon(Glyphs::ICON_SEARCH));
}

void AlbumArtistPage::onRetranslateUi() {	
	page_title_label_->setText(tr("Library"));

	album_tab_list_view_->setTabText(tr("ALBUMS"), TAB_ALBUMS);
	album_tab_list_view_->setTabText(tr("ARTISTS"), TAB_ARTISTS);
	album_tab_list_view_->setTabText(tr("YEAR"), TAB_YEAR);
}

void AlbumArtistPage::reload() {
	album_view_->reload();
	artist_view_->reload();

	dao::AlbumDao album_dao(qGuiDb.getDatabase());

	Q_FOREACH(auto category, album_dao.getCategories()) {
		album_tag_list_widget_->addTag(category);
	}

	auto years = album_dao.getYears();
    std::sort(years.begin(), years.end());
	Q_FOREACH(auto year, years) {
		year_tag_list_widget_->addTag(year, true);
	}
	if (!years.empty()) {
		year_tag_list_widget_->enableTag(years.last());
	}
	year_tag_list_widget_->sort();
}

