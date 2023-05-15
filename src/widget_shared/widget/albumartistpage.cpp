#include <widget/albumartistpage.h>

#include <QVBoxLayout>
#include <QStackedWidget>
#include <QStandardItemModel>
#include <QGraphicsOpacityEffect>
#include <QPainter>
#include <QApplication>
#include <QScrollBar>
#include <QPainterPath>
#include <QLabel>
#include <QMouseEvent>
#include <QLinearGradient>
#include <QBitmap>
#include <QLineEdit>
#include <QComboBox>
#include <QCompleter>
#include <QToolButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QListWidget>

#include <widget/artistview.h>
#include <widget/genre_view.h>
#include <widget/albumentity.h>
#include <widget/clickablelabel.h>
#include <widget/str_utilts.h>
#include <widget/albumview.h>
#include <widget/artistinfopage.h>
#include <widget/database.h>
#include <widget/ui_utilts.h>
#include <widget/taglistview.h>
#include <thememanager.h>

enum {
	TAB_ALBUM,
	TAB_ARTIST,
	TAB_GENRE,
};

GenrePage::GenrePage(QWidget* parent)
	: QFrame(parent) {
	auto genre_container_layout = new QVBoxLayout(this);

	auto f = font();
	genre_label_ = new ClickableLabel();
	f.setPointSize(qTheme.GetFontSize(15));
	f.setBold(true);
	genre_label_->setFont(f);

	auto* cevron_right = new QToolButton();
	cevron_right->setIcon(qTheme.GetFontIcon(Glyphs::ICON_CHEVRON_LEFT));

	auto* genre_combox_layout = new QHBoxLayout();
	auto horizontalSpacer_3 = new QSpacerItem(20, 50, QSizePolicy::Expanding, QSizePolicy::Expanding);
	genre_combox_layout->addWidget(genre_label_);
	genre_combox_layout->addWidget(cevron_right);
	genre_combox_layout->addSpacerItem(horizontalSpacer_3);

	genre_view_ = new GenreView(this);

	genre_view_->ShowAll();
	genre_view_->Refresh();

	genre_container_layout->addLayout(genre_combox_layout);
	genre_container_layout->addWidget(genre_view_, 1);

	(void)QObject::connect(genre_label_, &ClickableLabel::clicked, [this]() {
		emit goBackPage();
		});

	(void)QObject::connect(cevron_right, &QToolButton::clicked, [this]() {
		emit goBackPage();
		});
}

void GenrePage::SetGenre(const QString& genre) {
	genre_view_->SetGenre(genre);
	auto genre_label_text = genre;
	genre_label_text = genre_label_text.replace(0, 1, genre_label_text.at(0).toUpper());
	genre_label_->setText(genre_label_text);
	genre_view_->ShowAllAlbum();
	genre_view_->SetShowMode(SHOW_NORMAL);
	genre_view_->Refresh();
}

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
    f.setPointSize(qTheme.GetFontSize(14));
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
	list_view_->AddTab(tr("Albums"), TAB_ALBUM);
	list_view_->AddTab(tr("Artists"), TAB_ARTIST);
	list_view_->AddTab(tr("Genre"), TAB_GENRE);

	qTheme.SetTabTheme(list_view_);

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

	auto* category_label = new QLabel(tr("Category:"));
	category_label->setFont(f);

	const QStringList category_list = {		 
		tr("dsd"),
		tr("hires"), 
		tr("soundtrack"),
		tr("final fantasy"),
		tr("piano collections"),
		tr("vocal collection"), 
		tr("best"), tr("complete"),
		tr("collection") 
	};

	category_combo_box_ = new QComboBox();
	category_combo_box_->setObjectName(QString::fromUtf8("categoryComboBox"));

	category_combo_box_->addItem(tr("all"));
	Q_FOREACH(auto category, category_list) {
		category_combo_box_->addItem(category);
	}

	category_combo_box_->setCurrentIndex(0);	

	auto* tagListWidget = new QListWidget();
	tagListWidget->setDragEnabled(false);
	tagListWidget->setUniformItemSizes(true);
	tagListWidget->setFlow(QListView::LeftToRight);
	tagListWidget->setResizeMode(QListView::Adjust);
	tagListWidget->setFrameStyle(QFrame::StyledPanel);
	tagListWidget->setViewMode(QListView::IconMode);
	tagListWidget->setFixedHeight(100);	

	tagListWidget->setStyleSheet(qTEXT(
		"QListWidget {"
		"  border: none;"
		"} "
		"QListWidget::item {"
		"  border: 1px solid transparent;"
		"  border-radius: 18px;"
		"  background-color: transparent;"
		"}"
		"QListWidget::item:selected {"
		"  background-color: transparent;"
		"}"
	));

	auto addTag = [&](const QString &tag) {
		auto color = GenerateRandomColor();

		auto* item = new TagWidgetItem(tag, color, tagListWidget);
		
		auto f = font();
		f.setBold(true);
		f.setPointSize(qTheme.GetFontSize(10));
		item->setSizeHint(QSize(170, 40));

		auto* layout = new QHBoxLayout();
		auto* tag_label = new QLabel(tag);		
		tag_label->setFont(f);
		tag_label->setAlignment(Qt::AlignCenter);

		layout->addWidget(tag_label);
		layout->setSpacing(0);
		layout->setContentsMargins(0, 0, 0, 0);

		auto* widget = new QWidget();
		widget->setLayout(layout);
		widget->setStyleSheet(
			qSTR("border-radius: 18px; background-color: %1;").arg(color.name())
		);	
		tagListWidget->setItemWidget(item, widget);
		item->Enable();

		(void)QObject::connect(tagListWidget, &QListWidget::itemClicked, [tagListWidget, this](auto* item) {
			if (!item) {
				return;
			}

			auto* tag_item = dynamic_cast<TagWidgetItem*>(item);		
			if (!tag_item) {
				return;
			}
			tag_item->Enable();
			if (!tag_item->IsEnable()) {
				category_set_.remove(tag_item->GetTag());
			}
			else {
				category_set_.insert(tag_item->GetTag());
			}
			if (!category_set_.isEmpty()) {
				album_view_->FilterCategories(category_set_);
				album_view_->Update();
			}
			else {
				album_view_->ShowAll();
				album_view_->Update();
			}
			});
		/*(void)QObject::connect(deleteButton, &QPushButton::clicked, [tagListWidget, item]() {
			delete tagListWidget->takeItem(tagListWidget->row(item));
			});*/
	};

	Q_FOREACH(auto category, category_list) {
		addTag(category);
	}

	auto* tagLayout = new QVBoxLayout();	
	tagLayout->addWidget(tagListWidget);

	album_search_line_edit_ = new QLineEdit();
	album_search_line_edit_->setObjectName(QString::fromUtf8("albumSearchLineEdit"));
	QSizePolicy sizePolicy3(QSizePolicy::Fixed, QSizePolicy::Fixed);
	sizePolicy3.setHorizontalStretch(0);
	sizePolicy3.setVerticalStretch(0);
	sizePolicy3.setHeightForWidth(album_search_line_edit_->sizePolicy().hasHeightForWidth());
	album_search_line_edit_->setSizePolicy(sizePolicy3);
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

	auto clearAllTag = [tagListWidget]() {
		for (auto i = 0; i < tagListWidget->count(); ++i) {
			auto item = (TagWidgetItem*)tagListWidget->item(i);
			item->SetEnable(false);
		}
	};

	auto actionList = album_search_line_edit_->findChildren<QAction*>();
	if (!actionList.isEmpty()) {
		Q_FOREACH(auto * action, actionList) {
			if (action) {
				(void)QObject::connect(action, &QAction::triggered, [clearAllTag, this]() {
					album_view_->ShowAll();
					album_view_->Update();		
					clearAllTag();
					});
			}
		}
	}

	(void)QObject::connect(album_search_line_edit_, &QLineEdit::textChanged, [this, clearAllTag, album_completer](const auto& text) {
		clearAllTag();

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
	album_combox_layout->addWidget(category_label);
	album_combox_layout->addWidget(category_combo_box_);

	auto horizontalSpacer = new QSpacerItem(20, 50, QSizePolicy::Expanding, QSizePolicy::Expanding);
	album_combox_layout_1->addSpacerItem(horizontalSpacer);
	album_combox_layout_1->addLayout(album_combox_layout);

	album_frame_layout->addLayout(album_combox_layout_1);
	album_frame_layout->addLayout(tagLayout);
	album_frame_layout->addWidget(album_view_, 1);

	auto genre_stackwidget = new QStackedWidget(this);

	auto* container_frame = new QFrame();
	auto genre_container_layout = new QVBoxLayout(container_frame);

	auto *genre_page = new GenrePage();
	(void)QObject::connect(genre_page, &GenrePage::goBackPage, [genre_stackwidget]() {
		genre_stackwidget->setCurrentIndex(0);
		});

	genre_stackwidget->addWidget(container_frame);
	genre_stackwidget->addWidget(genre_page);

	auto* scroll_area = new QScrollArea();
	scroll_area->setWidgetResizable(true);
	scroll_area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	scroll_area->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

	genre_frame_ = new QFrame();
	genre_frame_->setObjectName(QString::fromUtf8("currentGenreViewFrame"));
	genre_frame_->setFrameShape(QFrame::StyledPanel);

	genre_frame_layout_ = new QVBoxLayout(genre_frame_);
	genre_frame_layout_->setSpacing(0);
	genre_frame_layout_->setObjectName(QString::fromUtf8("currentGenreViewFrameLayout"));
	genre_frame_layout_->setContentsMargins(0, 0, 0, 0);

	scroll_area->setWidget(genre_frame_);

	Q_FOREACH(auto genre, qDatabase.GetGenres()) {
		AddGenreList(genre_page, genre_stackwidget, genre);
	}

	genre_container_layout->addWidget(scroll_area);
	
	artist_frame_ = new QFrame();
	artist_frame_->setObjectName(QString::fromUtf8("currentArtistViewFrame"));
	artist_frame_->setFrameShape(QFrame::StyledPanel);
	auto* artist_frame_layout = new QVBoxLayout(artist_frame_);
	artist_frame_layout->setSpacing(0);
	artist_frame_layout->setObjectName(QString::fromUtf8("currentArtistViewFrameLayout"));
	artist_frame_layout->setContentsMargins(0, 0, 0, 0);

	artist_search_line_edit_ = new QLineEdit();
	artist_search_line_edit_->setObjectName(QString::fromUtf8("artistSearchLineEdit"));
	QSizePolicy sizePolicy4(QSizePolicy::Fixed, QSizePolicy::Fixed);
	sizePolicy3.setHorizontalStretch(0);
	sizePolicy3.setVerticalStretch(0);
	sizePolicy3.setHeightForWidth(artist_search_line_edit_->sizePolicy().hasHeightForWidth());
	artist_search_line_edit_->setSizePolicy(sizePolicy4);
	artist_search_line_edit_->setMinimumSize(QSize(180, 30));
	artist_search_line_edit_->setFocusPolicy(Qt::ClickFocus);
	artist_search_line_edit_->setClearButtonEnabled(true);
	artist_search_line_edit_->addAction(qTheme.GetFontIcon(Glyphs::ICON_SEARCH), QLineEdit::LeadingPosition);
	artist_search_line_edit_->setPlaceholderText(tr("Search Artist"));

	actionList = artist_search_line_edit_->findChildren<QAction*>();
	if (!actionList.isEmpty()) {
		Q_FOREACH(auto * action, actionList) {
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

	artist_frame_layout->addLayout(artist_combox_layout_1);
	artist_frame_layout->addWidget(artist_view_, 1);

	current_view->addWidget(album_frame_);
	current_view->addWidget(artist_frame_);
	current_view->addWidget(genre_stackwidget);

	auto* default_layout = new QHBoxLayout();
	default_layout->setSpacing(0);
	default_layout->setObjectName(QString::fromUtf8("default_layout"));
	default_layout->setContentsMargins(0, 0, 0, 0);
	default_layout->addWidget(current_view);

	vertical_layout_2->addLayout(horizontal_layout_5);
	vertical_layout_2->addLayout(default_layout, 1);

	(void)QObject::connect(album_view_, &AlbumView::RemoveAll,
		this, &AlbumArtistPage::Refresh);

	(void)QObject::connect(album_view_, &AlbumView::LoadCompleted,
		this, &AlbumArtistPage::Refresh);

	(void)QObject::connect(category_combo_box_, &QComboBox::textActivated, [this](const auto &category) {
		if (category != tr("all")) {
			album_view_->FilterCategories(category);
		}
		else {
			album_view_->ShowAll();
		}
		album_view_->Update();
		});

	(void)QObject::connect(list_view_, &AlbumTabListView::ClickedTable, [this, current_view, genre_stackwidget](auto table_id) {
		switch (table_id) {
			case TAB_ALBUM:
				current_view->setCurrentWidget(album_frame_);
				break;
			case TAB_ARTIST:
				current_view->setCurrentWidget(artist_frame_);
				break;
			case TAB_GENRE:
				current_view->setCurrentWidget(genre_stackwidget);
				break;
		}
		});

	current_view->setCurrentIndex(0);
	list_view_->SetCurrentTab(TAB_ALBUM);

	OnCurrentThemeChanged(qTheme.GetThemeColor());
}

void AlbumArtistPage::AddGenreList(GenrePage *page, QStackedWidget* stack, const QString &genre) {
	auto f = font();

	auto genre_label_text = genre;
	genre_label_text = genre_label_text.replace(0, 1, genre_label_text.at(0).toUpper());
	auto* genre_label = new ClickableLabel(genre_label_text);
	f.setPointSize(qTheme.GetFontSize(15));
	f.setBold(true);
	genre_label->setFont(f);	

	auto* cevron_right = new QToolButton();
	cevron_right->setIcon(qTheme.GetFontIcon(Glyphs::ICON_CHEVRON_RIGHT));

	auto *genre_combox_layout = new QHBoxLayout();
	auto horizontalSpacer_3 = new QSpacerItem(20, 50, QSizePolicy::Expanding, QSizePolicy::Expanding);
	genre_combox_layout->addWidget(genre_label);
	genre_combox_layout->addWidget(cevron_right);
	genre_combox_layout->addSpacerItem(horizontalSpacer_3);

	auto* genre_view = new GenreView(this);
	genre_view->SetGenre(genre);
	genre_view->ShowAll();
	genre_view->Refresh();
	genre_view->verticalScrollBar()->hide();
	genre_view->setFixedHeight(275);
	genre_view->SetShowMode(SHOW_NORMAL);

	(void)QObject::connect(genre_label, &ClickableLabel::clicked, [page, genre, stack, this]() {
		page->SetGenre(genre);
		stack->setCurrentIndex(1);
		});

	(void)QObject::connect(cevron_right, &QToolButton::clicked, [page, genre, stack, this]() {
		page->SetGenre(genre);
		stack->setCurrentIndex(1);
		});

	genre_page_list_.append(page);
	genre_list_.append(genre_view);

	genre_frame_layout_->addLayout(genre_combox_layout);
	genre_frame_layout_->addWidget(genre_view, 1);
}

void AlbumArtistPage::OnCurrentThemeChanged(ThemeColor theme_color) {
	album_view_->OnCurrentThemeChanged(theme_color);
	artist_view_->OnCurrentThemeChanged(theme_color);

	Q_FOREACH(auto * view, genre_list_) {
		view->OnCurrentThemeChanged(theme_color);
	}
	Q_FOREACH(auto* page, genre_page_list_) {
		page->view()->OnCurrentThemeChanged(theme_color);
	}

	qTheme.SetLineEditStyle(album_search_line_edit_, qTEXT("albumSearchLineEdit"));
	qTheme.SetLineEditStyle(artist_search_line_edit_, qTEXT("artistSearchLineEdit"));
	qTheme.SetComboBoxStyle(category_combo_box_, qTEXT("categoryComboBox"));
}

void AlbumArtistPage::OnThemeColorChanged(QColor backgroundColor, QColor color) {
	album_view_->OnThemeChanged(backgroundColor, color);
	artist_view_->OnThemeChanged(backgroundColor, color);

	Q_FOREACH(auto * view, genre_list_) {
		view->OnThemeChanged(backgroundColor, color);
	}
	Q_FOREACH(auto* page, genre_page_list_) {
		page->view()->OnThemeChanged(backgroundColor, color);
	}
}

void AlbumArtistPage::Refresh() {
	album_view_->Refresh();
	artist_view_->Refresh();
	Q_FOREACH(auto * view, genre_list_) {
		view->Refresh();
	}
	Q_FOREACH(auto * page, genre_page_list_) {
		page->view()->Refresh();
	}
}

