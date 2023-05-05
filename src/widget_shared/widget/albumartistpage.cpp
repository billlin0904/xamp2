#include <widget/albumartistpage.h>
#include <widget/artistview.h>

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

#include <widget/albumentity.h>
#include <widget/str_utilts.h>
#include <widget/albumview.h>
#include <widget/artistinfopage.h>
#include <widget/database.h>
#include <thememanager.h>

enum {
	TAB_ALBUM,
	TAB_ARTIST,
	TAB_GENRE,
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
    item->setSizeHint(QSize(160, 30));
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

	auto* categoryLabel = new QLabel(tr("Category:"));
	categoryLabel->setFont(f);

	category_combo_box_ = new QComboBox();
	category_combo_box_->setObjectName(QString::fromUtf8("categoryComboBox"));
	category_combo_box_->addItem(tr("all"));
	category_combo_box_->addItem(tr("dsd"));
	category_combo_box_->addItem(tr("hires"));
	category_combo_box_->addItem(tr("soundtrack"));
	category_combo_box_->addItem(tr("final fantasy"));
	category_combo_box_->addItem(tr("piano collections"));
	category_combo_box_->addItem(tr("vocal collection"));	
	category_combo_box_->addItem(tr("best"));
	category_combo_box_->addItem(tr("complete"));	
	category_combo_box_->addItem(tr("collection"));
	category_combo_box_->setCurrentIndex(0);	

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

	(void)QObject::connect(album_search_line_edit_, &QLineEdit::textChanged, [this, album_completer](const auto& text) {
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
	album_combox_layout->addWidget(categoryLabel);
	album_combox_layout->addWidget(category_combo_box_);

	auto horizontalSpacer = new QSpacerItem(20, 50, QSizePolicy::Expanding, QSizePolicy::Expanding);
	album_combox_layout_1->addSpacerItem(horizontalSpacer);
	album_combox_layout_1->addLayout(album_combox_layout);

	album_frame_layout->addLayout(album_combox_layout_1);
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

	(void)QObject::connect(list_view_, &AlbumTabListView::ClickedTable, [this, current_view](auto table_id) {
		switch (table_id) {
			case TAB_ALBUM:
				current_view->setCurrentWidget(album_frame_);
				break;
			case TAB_ARTIST:
				current_view->setCurrentWidget(artist_frame_);
				break;
		}
		});

	current_view->setCurrentIndex(0);
	list_view_->SetCurrentTab(TAB_ALBUM);

	OnCurrentThemeChanged(qTheme.GetThemeColor());
}

void AlbumArtistPage::OnCurrentThemeChanged(ThemeColor theme_color) {
	album_view_->OnCurrentThemeChanged(theme_color);
	artist_view_->OnCurrentThemeChanged(theme_color);

	// QStackedWidget設定background與border顏色與背景相同, 
	// 所以重新設定categoryComboBox的border顏色與背景色
	QString border_color;
	QString selection_background_color;
	QString on_selection_background_color;
	switch (theme_color) {
		case ThemeColor::LIGHT_THEME:	
			border_color = "#C9CDD0";
			selection_background_color = "#FAFAFA";
			on_selection_background_color = "#1e1d23";			
			break;
		case ThemeColor::DARK_THEME:
			border_color = "#455364";		
			selection_background_color = "#1e1d23";
			on_selection_background_color = "#9FCBFF";	
			break;
	}

	qTheme.SetLineEditStyle(album_search_line_edit_, qTEXT("albumSearchLineEdit"));
	qTheme.SetLineEditStyle(artist_search_line_edit_, qTEXT("artistSearchLineEdit"));

	category_combo_box_->setStyleSheet(qSTR(R"(
    QComboBox#categoryComboBox {
		background-color: %2;
		border: 1px solid %1;
	}
	QComboBox QAbstractItemView#categoryComboBox {
		background-color: %2;
	}
	QComboBox#categoryComboBox:on {
		selection-background-color: %3;
	}
    )").arg(border_color).arg(selection_background_color).arg(on_selection_background_color));
}

void AlbumArtistPage::OnThemeColorChanged(QColor backgroundColor, QColor color) {
	album_view_->OnThemeChanged(backgroundColor, color);
	artist_view_->OnThemeChanged(backgroundColor, color);	
}

void AlbumArtistPage::Refresh() {
	album_view_->Refresh();
	artist_view_->Refresh();
}

void AlbumArtistPage::SetArtistId(const QString& artist, const QString& cover_id, int32_t artist_id) {

}
