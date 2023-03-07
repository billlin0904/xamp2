#include <QVBoxLayout>
#include <QStackedWidget>
#include <QStandardItemModel>

#include <widget/str_utilts.h>
#include <widget/albumview.h>
#include <widget/artistinfopage.h>

#include <thememanager.h>
#include <widget/albumartistpage.h>

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
	, artist_view_(new ArtistInfoPage(this)) {
	auto* vertical_layout_2 = new QVBoxLayout(this);

	vertical_layout_2->setSpacing(0);
	vertical_layout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));

	auto * horizontal_layout_5 = new QHBoxLayout();
	horizontal_layout_5->setSpacing(6);
	horizontal_layout_5->setObjectName(QString::fromUtf8("horizontalLayout_5"));
	auto *horizontal_spacer_6 = new QSpacerItem(50, 50, QSizePolicy::Expanding, QSizePolicy::Expanding);
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

	current_view->addWidget(album_view_);
	current_view->addWidget(artist_view_);

	auto* default_layout = new QHBoxLayout();
	default_layout->setSpacing(0);
	default_layout->setObjectName(QString::fromUtf8("default_layout"));
	default_layout->setContentsMargins(0, 0, 0, 0);
	default_layout->addWidget(current_view);

	vertical_layout_2->addLayout(horizontal_layout_5);
	vertical_layout_2->addLayout(default_layout);
	vertical_layout_2->setStretch(1, 2);

	(void)QObject::connect(album_view_, &AlbumView::RemoveAll,
		this, &AlbumArtistPage::Refresh);

	(void)QObject::connect(album_view_, &AlbumView::LoadCompleted,
		this, &AlbumArtistPage::Refresh);

	(void)QObject::connect(list_view_, &AlbumTabListView::ClickedTable, [this, current_view](auto table_id) {
		current_view->setCurrentIndex(table_id);
		});

	setStyleSheet(qTEXT("background-color: transparent"));
	current_view->setCurrentIndex(0);
	list_view_->SetCurrentTab(TAB_ALBUM);
}

void AlbumArtistPage::OnCurrentThemeChanged(ThemeColor theme_color) {
	album_view_->OnCurrentThemeChanged(theme_color);
	artist_view_->album()->OnCurrentThemeChanged(theme_color);
}

void AlbumArtistPage::OnThemeColorChanged(QColor backgroundColor, QColor color) {
	album_view_->OnThemeChanged(backgroundColor, color);
	artist_view_->album()->OnThemeChanged(backgroundColor, color);
}

void AlbumArtistPage::SetArtistId(const QString& artist, const QString& cover_id, int32_t artist_id) {
	artist_view_->SetArtistId(artist, cover_id, artist_id);
}

void AlbumArtistPage::Refresh() {
	album_view_->Refresh();
}

void AlbumArtistPage::OnThemeChanged(QColor backgroundColor, QColor color) {
}
