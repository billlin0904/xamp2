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
#include <QComboBox>

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

	auto* comboxLayout_1 = new QHBoxLayout();
	auto* comboxLayout = new QHBoxLayout();
	auto* categoryLabel = new QLabel(tr("Category:"));

	auto* categoryComboBox = new QComboBox();
	categoryComboBox->addItem(tr("all"));
	categoryComboBox->addItem(tr("soundtrack"));
	categoryComboBox->addItem(tr("final fantasy"));
	categoryComboBox->addItem(tr("piano collections"));
	categoryComboBox->addItem(tr("vocal collection"));	
	categoryComboBox->addItem(tr("best"));
	categoryComboBox->addItem(tr("complete"));
	categoryComboBox->setCurrentIndex(0);

	comboxLayout->addWidget(categoryLabel);
	comboxLayout->addWidget(categoryComboBox);

	auto horizontalSpacer = new QSpacerItem(20, 50, QSizePolicy::Expanding, QSizePolicy::Expanding);
	comboxLayout_1->addSpacerItem(horizontalSpacer);
	comboxLayout_1->addLayout(comboxLayout);

	album_frame_layout->addLayout(comboxLayout_1);
	album_frame_layout->addWidget(album_view_, 1);

	current_view->addWidget(album_frame_);
	current_view->addWidget(artist_view_);

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

	(void)QObject::connect(categoryComboBox, &QComboBox::textActivated, [this](const auto &category) {
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
				current_view->setCurrentWidget(artist_view_);
				break;
		}
		});

	current_view->setCurrentIndex(0);
	list_view_->SetCurrentTab(TAB_ALBUM);
}

void AlbumArtistPage::OnCurrentThemeChanged(ThemeColor theme_color) {
	album_view_->OnCurrentThemeChanged(theme_color);
	artist_view_->OnCurrentThemeChanged(theme_color);
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
