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

#include <widget/ui_utilts.h>
#include <widget/artistview.h>
#include <widget/genre_view.h>
#include <widget/clickablelabel.h>
#include <widget/str_utilts.h>
#include <widget/albumview.h>
#include <widget/artistinfopage.h>
#include <widget/database.h>
#include <widget/taglistview.h>
#include <widget/genre_view_page.h>
#include <widget/flowlayout.h>
#include <thememanager.h>

enum {
	TAB_ALBUMS,
	TAB_ARTISTS,
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
    item->setSizeHint(QSize(130, 30));
	item->setTextAlignment(Qt::AlignCenter);
	auto f = item->font();
    f.setPointSize(qTheme.fontSize(20));
	f.setBold(true);
	item->setFont(f);
	model_.appendRow(item);
}

void AlbumTabListView::setCurrentTab(int tab_id) {
	setCurrentIndex(model_.index(tab_id, 0));
}

AlbumArtistPage::AlbumArtistPage(QWidget* parent)
	: QFrame(parent)
	, list_view_(new AlbumTabListView(this))
	, album_view_(new AlbumView(this))
	, artist_view_(new ArtistView(this))
	, artist_info_view_(new ArtistInfoPage(this)) {
	album_view_->reload();
	artist_view_->reload();

	auto* vertical_layout_2 = new QVBoxLayout(this);
	vertical_layout_2->setSpacing(0);
	vertical_layout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));

	auto f = font();
	auto* title_label = new QLabel(tr("Library"), this);
	f.setBold(true);
	f.setPointSize(qTheme.fontSize(40));
	title_label->setFont(f);
	vertical_layout_2->addWidget(title_label);

	auto* horizontal_layout_5 = new QHBoxLayout();
	horizontal_layout_5->setSpacing(6);
	horizontal_layout_5->setObjectName(QString::fromUtf8("horizontalLayout_5"));

	//auto* horizontal_spacer_6 = new QSpacerItem(50, 50, QSizePolicy::Expanding, QSizePolicy::Expanding);
	//horizontal_layout_5->addItem(horizontal_spacer_6);

	list_view_->setObjectName(QString::fromUtf8("albumTab"));
	list_view_->addTab(qTR("ALBUM"), TAB_ALBUMS);
	list_view_->addTab(qTR("ARTISTS"), TAB_ARTISTS);
	list_view_->addTab(qTR("YEAR"), TAB_YEAR);

	qTheme.setAlbumNaviBarTheme(list_view_);

	constexpr QSizePolicy size_policy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	list_view_->setSizePolicy(size_policy);
	list_view_->setMinimumSize(500, 40);
	list_view_->setMaximumHeight(40);
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

	f.setBold(true);
	f.setPointSize(qTheme.fontSize(10));

	auto title_category_list = qMainDb.getCategories();

	album_tag_list_widget_ = new TagListView();
	album_tag_list_widget_->setListViewFixedHeight(70);

	std::ranges::sort(title_category_list, [](const auto& left, const auto& right) {
		return left.length() < right.length();
		});
	
	Q_FOREACH(auto category, title_category_list) {
		album_tag_list_widget_->addTag(category);
	}
	album_tag_list_widget_->addTag(qTR("All"), true);

	(void)QObject::connect(album_tag_list_widget_, &TagListView::tagChanged, [this](const auto& tags) {
		if (tags.isEmpty()) {
			return;
		}
		if (tags.contains(qTR("All"))) {
			album_view_->showAll();
			album_tag_list_widget_->disableAllTag(qTR("All"));
		}
		else {
			album_view_->filterCategories(tags);
		}
		album_view_->albumViewPage()->hide();
		album_view_->reload();
		});

	(void)QObject::connect(album_tag_list_widget_, &TagListView::tagClear, [this]() {
		album_view_->showAll();
		album_view_->reload();
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
	album_search_line_edit_->addAction(qTheme.fontIcon(Glyphs::ICON_SEARCH), QLineEdit::TrailingPosition);
	album_search_line_edit_->setPlaceholderText(qTR("Search Album/Artist"));
	
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
					album_tag_list_widget_->clearTag();
					});
			}
		}
	}

	(void)QObject::connect(album_search_line_edit_, &QLineEdit::textChanged, [this, album_completer](const auto& text) {
		album_tag_list_widget_->clearTag();

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

	auto* tags_label = new QLabel(qTR("Category"));
	f.setPointSize(qTheme.fontSize(9));
	tags_label->setFont(f);

	album_frame_layout->addWidget(tags_label);

	album_frame_layout->addWidget(album_tag_list_widget_);
	album_frame_layout->addWidget(album_view_, 1);
	
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
	artist_search_line_edit_->addAction(qTheme.fontIcon(Glyphs::ICON_SEARCH), QLineEdit::TrailingPosition);
	artist_search_line_edit_->setPlaceholderText(qTR("search Artist"));

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
		if (tags.contains(qTR("All"))) {
			artist_view_->showAll();
			artist_tag_list_widget_->disableAllTag(qTR("All"));
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
	Q_FOREACH (auto year, qMainDb.getYears()) {
		year_tag_list_widget_->addTag(year, true);
	}
	year_frame_layout->addWidget(year_tag_list_widget_);
	year_frame_layout->addWidget(year_view_, 1);

	(void)QObject::connect(year_tag_list_widget_, &TagListView::tagChanged, [this](const QSet<QString>& tags) {
		if (tags.isEmpty()) {
			return;
		}
		if (tags.contains(qTR("All"))) {
			year_view_->showAll();
			year_tag_list_widget_->disableAllTag(qTR("All"));
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

	(void)QObject::connect(list_view_, &AlbumTabListView::clickedTable, [this, current_view](auto table_id) {
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
			default: 
				break;
		}
		});

	current_view->setCurrentIndex(0);
	list_view_->setCurrentTab(TAB_ALBUMS);

	(void)QObject::connect(album_view_, &AlbumView::removeAll, [this]() {
		album_tag_list_widget_->clearTag();
		year_tag_list_widget_->clearTag();

		artist_view_->reload();
		year_view_->reload();
		reload();
		});

	onThemeChangedFinished(qTheme.themeColor());
}

void AlbumArtistPage::onThemeChangedFinished(ThemeColor theme_color) {
	album_view_->onThemeChangedFinished(theme_color);
	artist_view_->onThemeChangedFinished(theme_color);
	year_view_->onThemeChangedFinished(theme_color);

	album_tag_list_widget_->onThemeChangedFinished(theme_color);
	artist_tag_list_widget_->onThemeChangedFinished(theme_color);

	qTheme.setLineEditStyle(album_search_line_edit_, qTEXT("albumSearchLineEdit"));
	qTheme.setLineEditStyle(artist_search_line_edit_, qTEXT("artistSearchLineEdit"));
}

void AlbumArtistPage::onThemeColorChanged(QColor background_color, QColor color) {
	album_view_->onThemeColorChanged(background_color, color);
	artist_view_->onThemeChanged(background_color, color);
	year_view_->onThemeColorChanged(background_color, color);
	album_tag_list_widget_->onThemeColorChanged(background_color, color);
	artist_tag_list_widget_->onThemeColorChanged(background_color, color);
}

void AlbumArtistPage::reload() {
	album_view_->reload();
	artist_view_->reload();

	Q_FOREACH(auto category, qMainDb.getCategories()) {
		album_tag_list_widget_->addTag(category);
	}

	auto years = qMainDb.getYears();
	std::ranges::sort(years);
	Q_FOREACH(auto year, years) {
		year_tag_list_widget_->addTag(year, true);
	}
	if (!years.empty()) {
		year_tag_list_widget_->enableTag(years.last());
	}
	year_tag_list_widget_->sort();
}

