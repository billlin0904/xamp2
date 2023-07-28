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
#include <QToolButton>
#include <QScrollArea>
#include <QTimer>

#include <widget/ui_utilts.h>
#include <widget/artistview.h>
#include <widget/genre_view.h>
#include <widget/clickablelabel.h>
#include <widget/str_utilts.h>
#include <widget/albumview.h>
#include <widget/artistinfopage.h>
#include <widget/database.h>
#include <widget/processindicator.h>
#include <widget/taglistview.h>
#include <widget/genre_view_page.h>
#include <thememanager.h>

enum {
	TAB_ALBUMS,
	TAB_ARTISTS,
	TAB_GENRE,
	TAB_YEAR,
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
	setIconSize(qTheme.GetTabIconSize());
	setFlow(QListView::Flow::LeftToRight);

	(void)QObject::connect(this, &QListView::clicked, [this](auto index) {
		auto table_id = index.data(Qt::UserRole + 1).toInt();
		emit ClickedTable(table_id);
		});

	(void)QObject::connect(&model_, &QStandardItemModel::itemChanged, [this](auto item) {
		auto table_id = item->data(Qt::UserRole + 1).toInt();
		auto table_name = item->data(Qt::DisplayRole).toString();
		emit TableNameChanged(table_id, table_name);
		});
}

void AlbumTabListView::AddTab(const QString& name, int tab_id) {
	auto* item = new QStandardItem(name);
	item->setData(tab_id);
    item->setSizeHint(QSize(90, 30));
	item->setTextAlignment(Qt::AlignCenter);
	auto f = item->font();
    f.setPointSize(qTheme.GetFontSize(12));
	f.setBold(true);
	item->setFont(f);
	model_.appendRow(item);
}

void AlbumTabListView::SetCurrentTab(int tab_id) {
	setCurrentIndex(model_.index(tab_id, 0));
}

AlbumArtistPage::AlbumArtistPage(QWidget* parent)
	: QFrame(parent)
	, list_view_(new AlbumTabListView(this))
	, album_view_(new AlbumView(this))
	, artist_view_(new ArtistView(this))
	, artist_info_view_(new ArtistInfoPage(this)) {
	album_view_->Refresh();
	album_view_->Update();
	artist_view_->Refresh();
	artist_view_->Update();

	auto* vertical_layout_2 = new QVBoxLayout(this);

	vertical_layout_2->setSpacing(0);
	vertical_layout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));

	auto* horizontal_layout_5 = new QHBoxLayout();
	horizontal_layout_5->setSpacing(6);
	horizontal_layout_5->setObjectName(QString::fromUtf8("horizontalLayout_5"));
	auto* horizontal_spacer_6 = new QSpacerItem(50, 50, QSizePolicy::Expanding, QSizePolicy::Expanding);
	horizontal_layout_5->addItem(horizontal_spacer_6);
	list_view_->setObjectName(QString::fromUtf8("albumTab"));
	list_view_->AddTab(tr("Albums"), TAB_ALBUMS);
	list_view_->AddTab(tr("Artists"), TAB_ARTISTS);
	list_view_->AddTab(tr("Genre"), TAB_GENRE);
	list_view_->AddTab(tr("Year"), TAB_YEAR);

	qTheme.SetAlbumNaviBarTheme(list_view_);

	const QSizePolicy size_policy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	list_view_->setSizePolicy(size_policy);
	list_view_->setMinimumSize(500, 30);
	list_view_->setMaximumHeight(50);
	horizontal_layout_5->addWidget(list_view_);
	auto* horizontal_spacer_7 = new QSpacerItem(50, 50, QSizePolicy::Expanding, QSizePolicy::Expanding);
	horizontal_layout_5->addItem(horizontal_spacer_7);

	auto* current_view = new QStackedWidget();
	current_view->setObjectName(QString::fromUtf8("currentView"));
	current_view->setFrameShadow(QFrame::Plain);
	current_view->setLineWidth(0);

	album_frame_ = new QFrame();
	album_frame_->setObjectName(QString::fromUtf8("currentAlbumViewFrame"));
	album_frame_->setFrameShape(QFrame::StyledPanel);

	auto* album_frame_layout = new QVBoxLayout(album_frame_);
	album_frame_layout->setSpacing(0);
	album_frame_layout->setObjectName(QString::fromUtf8("currentAlbumViewFrameLayout"));
	album_frame_layout->setContentsMargins(0, 0, 0, 0);

	auto* album_combox_layout_1 = new QHBoxLayout();
	auto* album_combox_layout = new QHBoxLayout();

	auto f = font();
	f.setBold(true);
	f.setPointSize(qTheme.GetFontSize(10));

	auto title_category_list = qDatabase.GetCategories();

	album_tag_list_widget_ = new TagListView();
	album_tag_list_widget_->SetListViewFixedHeight(70);

	std::sort(title_category_list.begin(), title_category_list.end(), [](const auto& left, const auto& right) {
		return left.length() < right.length();
		});
	
	Q_FOREACH(auto category, title_category_list) {
		album_tag_list_widget_->AddTag(category);
	}

	(void)QObject::connect(album_tag_list_widget_, &TagListView::TagChanged, [this](const auto& tags) {
		//auto indicator = MakeProcessIndicator(this);
		//indicator->StartAnimation();		
		//Delay(1);
		album_view_->FilterCategories(tags);
		album_view_->Update();
		});

	(void)QObject::connect(album_tag_list_widget_, &TagListView::TagClear, [this]() {
		album_view_->ShowAll();
		album_view_->Update();
		});

	album_search_line_edit_ = new QLineEdit();
	album_search_line_edit_->setObjectName(QString::fromUtf8("albumSearchLineEdit"));
	QSizePolicy size_policy3(QSizePolicy::Fixed, QSizePolicy::Fixed);
	size_policy3.setHorizontalStretch(0);
	size_policy3.setVerticalStretch(0);
	size_policy3.setHeightForWidth(album_search_line_edit_->sizePolicy().hasHeightForWidth());
	album_search_line_edit_->setSizePolicy(size_policy3);
	album_search_line_edit_->setMinimumSize(QSize(180, 30));
	album_search_line_edit_->setFocusPolicy(Qt::ClickFocus);
	album_search_line_edit_->setClearButtonEnabled(true);
	album_search_line_edit_->addAction(qTheme.GetFontIcon(Glyphs::ICON_SEARCH), QLineEdit::LeadingPosition);
	album_search_line_edit_->setPlaceholderText(tr("Search Album"));
	
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
					album_view_->ShowAll();
					album_view_->Update();		
					album_tag_list_widget_->ClearTag();
					});
			}
		}
	}

	(void)QObject::connect(album_search_line_edit_, &QLineEdit::textChanged, [this, album_completer](const auto& text) {
		album_tag_list_widget_->ClearTag();

		const auto items = album_model_->findItems(text, Qt::MatchExactly);
		if (!items.isEmpty()) {
			return;
		}
		if (album_model_->rowCount() >= kMaxCompletionCount) {
			album_model_->removeRows(0, kMaxCompletionCount - album_model_->rowCount() + 1);
		}

		album_model_->appendRow(new QStandardItem(text));
		album_completer->setModel(album_model_);
		album_completer->setCompletionPrefix(text);

		album_view_->OnSearchTextChanged(text);
		album_view_->Update();
		});

	album_combox_layout->addWidget(album_search_line_edit_);	

	auto horizontalSpacer = new QSpacerItem(20, 50, QSizePolicy::Expanding, QSizePolicy::Expanding);
	album_combox_layout_1->addSpacerItem(horizontalSpacer);

	auto* sort_by_button = new QToolButton();
	sort_by_button->setText(tr("Sort by years"));
	sort_by_button->setObjectName(QString::fromUtf8("sortByButton"));
	sort_by_button->setFocusPolicy(Qt::NoFocus);
	sort_by_button->setIcon(qTheme.GetFontIcon(Glyphs::ICON_SORT_DOWN));
	sort_by_button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	sort_by_button->setAutoRaise(true);
	sort_by_button->setPopupMode(QToolButton::MenuButtonPopup);

	album_combox_layout_1->addWidget(sort_by_button, 1);
	auto* sort_menu = new QMenu(this);
	auto *sort_year_act = sort_menu->addAction(tr("Sort By Year"));
	auto* sort_playback_count_act = sort_menu->addAction(tr("Sort By PlayCunt"));
	(void)QObject::connect(sort_year_act, &QAction::triggered, [this, sort_by_button]() {
		album_view_->SortYears();
		album_view_->Update();
		sort_by_button->setText(tr("Sort By Year"));
		album_tag_list_widget_->ClearTag();
		});
	(void)QObject::connect(sort_playback_count_act, &QAction::triggered, [this, sort_by_button]() {
		sort_by_button->setText(tr("Sort By PlayCunt"));
		album_tag_list_widget_->ClearTag();
		});
	sort_by_button->setMenu(sort_menu);

	album_combox_layout_1->addLayout(album_combox_layout);
	album_frame_layout->addLayout(album_combox_layout_1);

	auto* tags_label = new QLabel(tr("Category"));
	f.setPointSize(qTheme.GetFontSize(12));
	tags_label->setFont(f);

	album_frame_layout->addWidget(tags_label);

	album_frame_layout->addWidget(album_tag_list_widget_);
	album_frame_layout->addWidget(album_view_, 1);

	genre_stackwidget_ = new GenreViewPage(this);
	auto genres_list = qDatabase.GetGenres();
	std::sort(genres_list.begin(), genres_list.end());

	Q_FOREACH(auto genre, genres_list) {
		genre_stackwidget_->AddGenre(genre);
	}
	
	artist_frame_ = new QFrame();
	artist_frame_->setObjectName(QString::fromUtf8("currentArtistViewFrame"));
	artist_frame_->setFrameShape(QFrame::StyledPanel);
	auto* artist_frame_layout = new QVBoxLayout(artist_frame_);
	artist_frame_layout->setSpacing(0);
	artist_frame_layout->setObjectName(QString::fromUtf8("currentArtistViewFrameLayout"));
	artist_frame_layout->setContentsMargins(0, 0, 0, 0);

	artist_search_line_edit_ = new QLineEdit();
	artist_search_line_edit_->setObjectName(QString::fromUtf8("artistSearchLineEdit"));
	QSizePolicy size_policy4(QSizePolicy::Fixed, QSizePolicy::Fixed);
	size_policy3.setHorizontalStretch(0);
	size_policy3.setVerticalStretch(0);
	size_policy3.setHeightForWidth(artist_search_line_edit_->sizePolicy().hasHeightForWidth());
	artist_search_line_edit_->setSizePolicy(size_policy4);
	artist_search_line_edit_->setMinimumSize(QSize(180, 30));
	artist_search_line_edit_->setFocusPolicy(Qt::ClickFocus);
	artist_search_line_edit_->setClearButtonEnabled(true);
	artist_search_line_edit_->addAction(qTheme.GetFontIcon(Glyphs::ICON_SEARCH), QLineEdit::LeadingPosition);
	artist_search_line_edit_->setPlaceholderText(tr("Search Artist"));

	action_list = artist_search_line_edit_->findChildren<QAction*>();
	if (!action_list.isEmpty()) {
		Q_FOREACH(auto * action, action_list) {
			if (action) {
				(void)QObject::connect(action, &QAction::triggered, [this]() {
					artist_view_->ShowAll();
					artist_view_->Update();
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
			return;
		}
		if (artist_model_->rowCount() >= kMaxCompletionCount) {
			artist_model_->removeRows(0, kMaxCompletionCount - artist_model_->rowCount() + 1);
		}

		artist_model_->appendRow(new QStandardItem(text));
		artist_completer->setModel(artist_model_);
		artist_completer->setCompletionPrefix(text);

		artist_view_->OnSearchTextChanged(text);
		artist_view_->Update();
		});

	auto horizontalSpacer_1 = new QSpacerItem(20, 50, QSizePolicy::Expanding, QSizePolicy::Expanding);
	auto* artist_combox_layout_1 = new QHBoxLayout();
	auto* artist_combox_layout = new QHBoxLayout();
	artist_combox_layout->addWidget(artist_search_line_edit_);
	artist_combox_layout_1->addSpacerItem(horizontalSpacer_1);
	artist_combox_layout_1->addLayout(artist_combox_layout);

	QStringList artist_category_list;
	for (auto i = 0; i < 26; ++i) {
		artist_category_list.append(QString('A' + i));
	}

	artist_tag_list_widget_ = new TagListView();

	Q_FOREACH(auto category, artist_category_list) {
		artist_tag_list_widget_->AddTag(category, true);
	}

	artist_frame_layout->addLayout(artist_combox_layout_1);

	artist_frame_layout->addWidget(artist_tag_list_widget_);
	artist_frame_layout->addWidget(artist_view_, 1);

	current_view->addWidget(album_frame_);
	current_view->addWidget(artist_frame_);
	current_view->addWidget(genre_stackwidget_);

	auto* default_layout = new QHBoxLayout();
	default_layout->setSpacing(0);
	default_layout->setObjectName(QString::fromUtf8("default_layout"));
	default_layout->setContentsMargins(0, 0, 0, 0);
	default_layout->addWidget(current_view);

	vertical_layout_2->addLayout(horizontal_layout_5);
	vertical_layout_2->addLayout(default_layout, 1);

	(void)QObject::connect(album_view_, &AlbumView::RemoveAll, [this](){
		album_tag_list_widget_->ClearTag();
		year_tag_list_widget_->ClearTag();
		genre_stackwidget_->Clear();
		Refresh();
	});

	(void)QObject::connect(album_view_, &AlbumView::LoadCompleted,
		this, &AlbumArtistPage::Refresh);

	(void)QObject::connect(artist_tag_list_widget_, &TagListView::TagChanged, [this](const auto& tags) {
		artist_view_->FilterAritstName(tags);
		artist_view_->Update();
		});

	(void)QObject::connect(artist_tag_list_widget_, &TagListView::TagClear, [this]() {
		artist_view_->ShowAll();
		artist_view_->Update();
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
	Q_FOREACH (auto year, qDatabase.GetYears()) {
		year_tag_list_widget_->AddTag(year, true);
	}	
	year_frame_layout->addWidget(year_tag_list_widget_);
	year_frame_layout->addWidget(year_view_, 1);

	(void)QObject::connect(year_tag_list_widget_, &TagListView::TagChanged, [this](const auto& tags) {
		year_view_->FilterYears(tags);
		year_view_->Update();
		});

	(void)QObject::connect(year_tag_list_widget_, &TagListView::TagClear, [this]() {
		year_view_->ShowAll();
		year_view_->Update();
		});

	current_view->addWidget(year_frame_);

	(void)QObject::connect(list_view_, &AlbumTabListView::ClickedTable, [this, current_view](auto table_id) {
		switch (table_id) {
			case TAB_ALBUMS:
				current_view->setCurrentWidget(album_frame_);
				break;
			case TAB_ARTISTS:
				current_view->setCurrentWidget(artist_frame_);
				break;
			case TAB_GENRE:
				current_view->setCurrentWidget(genre_stackwidget_);
				break;
			case TAB_YEAR:
				current_view->setCurrentWidget(year_frame_);
				break;
		}
		});

	current_view->setCurrentIndex(0);
	list_view_->SetCurrentTab(TAB_ALBUMS);

	OnCurrentThemeChanged(qTheme.GetThemeColor());
}

void AlbumArtistPage::OnCurrentThemeChanged(ThemeColor theme_color) {
	album_view_->OnCurrentThemeChanged(theme_color);
	artist_view_->OnCurrentThemeChanged(theme_color);
	year_view_->OnCurrentThemeChanged(theme_color);

	genre_stackwidget_->OnCurrentThemeChanged(theme_color);
	album_tag_list_widget_->OnCurrentThemeChanged(theme_color);
	artist_tag_list_widget_->OnCurrentThemeChanged(theme_color);

	qTheme.SetLineEditStyle(album_search_line_edit_, qTEXT("albumSearchLineEdit"));
	qTheme.SetLineEditStyle(artist_search_line_edit_, qTEXT("artistSearchLineEdit"));
}

void AlbumArtistPage::OnThemeColorChanged(QColor backgroundColor, QColor color) {
	album_view_->OnThemeChanged(backgroundColor, color);
	artist_view_->OnThemeChanged(backgroundColor, color);
	year_view_->OnThemeChanged(backgroundColor, color);
	album_tag_list_widget_->OnThemeColorChanged(backgroundColor, color);
	artist_tag_list_widget_->OnThemeColorChanged(backgroundColor, color);
	genre_stackwidget_->OnThemeColorChanged(backgroundColor, color);
}

void AlbumArtistPage::Refresh() {
	album_view_->Refresh();
	artist_view_->Refresh();	
	genre_stackwidget_->Refresh();

	Q_FOREACH(auto category, qDatabase.GetCategories()) {
		album_tag_list_widget_->AddTag(category);
	}

	auto years = qDatabase.GetYears();
	Q_FOREACH(auto year, years) {
		year_tag_list_widget_->AddTag(year, true);
	}
	if (!years.empty()) {
		year_tag_list_widget_->EnableTag(years.first());
	}	

	auto genres_list = qDatabase.GetGenres();
	std::sort(genres_list.begin(), genres_list.end());

	Q_FOREACH(auto genre, genres_list) {
		genre_stackwidget_->AddGenre(genre);
	}
}

