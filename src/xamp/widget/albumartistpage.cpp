#include <QVBoxLayout>
#include <QStackedWidget>
#include <QStandardItemModel>
#include <QPainter>
#include <QApplication>
#include <QScrollBar>
#include <QPainterPath>
#include <QLabel>
#include <QMouseEvent>
#include <QLinearGradient>
#include <QBitmap>

#include <widget/albumentity.h>
#include <widget/str_utilts.h>
#include <widget/albumview.h>
#include <widget/artistinfopage.h>
#include <widget/database.h>
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
	, artist_view_(new ArtistView(this))
	, artist_info_view_(new ArtistInfoPage(this)) {
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
		switch (table_id) {
			case TAB_ALBUM:
				current_view->setCurrentWidget(album_view_);
				break;
			case TAB_ARTIST:
				current_view->setCurrentWidget(artist_view_);
				break;
		}
		});

	setStyleSheet(qTEXT("background-color: transparent"));
	current_view->setCurrentIndex(0);
	list_view_->SetCurrentTab(TAB_ALBUM);
}

void AlbumArtistPage::OnCurrentThemeChanged(ThemeColor theme_color) {
	album_view_->OnCurrentThemeChanged(theme_color);
}

void AlbumArtistPage::OnThemeColorChanged(QColor backgroundColor, QColor color) {
	album_view_->OnThemeChanged(backgroundColor, color);
	artist_view_->OnThemeChanged(backgroundColor, color);
}

void AlbumArtistPage::SetArtistId(const QString& artist, const QString& cover_id, int32_t artist_id) {
	
}

void AlbumArtistPage::Refresh() {
	album_view_->Refresh();
	artist_view_->Refresh();
}

ArtistStyledItemDelegate::ArtistStyledItemDelegate(QObject* parent)
	: QStyledItemDelegate(parent) {
}

void ArtistStyledItemDelegate::SetTextColor(QColor color) {
	text_color_ = color;
}

void ArtistStyledItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
	auto* style = option.widget ? option.widget->style() : QApplication::style();

	auto artist = index.model()->data(index.model()->index(index.row(), 0)).toString();
	auto artistId = index.model()->data(index.model()->index(index.row(), 1)).toString();
	auto artistCoverId = index.model()->data(index.model()->index(index.row(), 2)).toString();
	auto first_char = index.model()->data(index.model()->index(index.row(), 3)).toString();

	const auto default_cover_size = qTheme.GetDefaultCoverSize();
	const QRect cover_rect(option.rect.left() + 10,
		option.rect.top() + 10,
		default_cover_size.width(),
		default_cover_size.height());

	painter->setRenderHints(QPainter::Antialiasing, true);
	painter->setRenderHints(QPainter::SmoothPixmapTransform, true);
	painter->setRenderHints(QPainter::TextAntialiasing, true);

	int size = cover_rect.height() - 4;
	QRect rect(cover_rect.x() + 2, cover_rect.y() + 2, size, size);
	painter->setPen(Qt::NoPen);
	painter->setBrush(Qt::gray);
	painter->drawEllipse(rect);
	
	auto font = painter->font();

	if (!artistCoverId.isEmpty()) {
		auto artist_cover = qAlbumCoverCache.GetCover(artistCoverId);
		painter->drawPixmap(rect, image_utils::RoundImage(artist_cover, size / 2));
	} else {				
		painter->setPen(Qt::white);
		font.setPointSize(qTheme.GetFontSize(70));
		font.setBold(true);
		painter->setFont(font);		
		painter->drawText(rect, Qt::AlignCenter, first_char);
	}

	painter->setPen(text_color_);
	font.setBold(false);
	font.setPointSize(qTheme.GetFontSize(8));
	painter->setFont(font);
	auto album_artist_text_width = default_cover_size.width();
	QRect artist_text_rect(option.rect.left() + 10,
		option.rect.top() + default_cover_size.height() + 15,
		album_artist_text_width,
		15);
	QFontMetrics album_metrics(font);
	painter->drawText(artist_text_rect, Qt::AlignVCenter,
		album_metrics.elidedText(artist, Qt::ElideRight, album_artist_text_width));
}

QSize ArtistStyledItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
	auto result = QStyledItemDelegate::sizeHint(option, index);
	const auto default_cover = qTheme.GetDefaultCoverSize();
	result.setWidth(default_cover.width() + 30);
	result.setHeight(default_cover.height() + 80);
	return result;
}

bool ArtistStyledItemDelegate::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) {
	if (event->type() == QEvent::MouseButtonPress) {
		auto* mouse_event = static_cast<QMouseEvent*>(event);
		if (mouse_event->button() == Qt::LeftButton) {
			emit EnterAlbumView(index);
		}
	}
	return true;
}

ArtistViewPage::ArtistViewPage(QWidget* parent)
	: QFrame(parent) {
	setObjectName(QString::fromUtf8("artistView"));
	setFrameStyle(QFrame::StyledPanel);

	close_button_ = new QPushButton(this);
	close_button_->setObjectName(qTEXT("albumViewPageCloseButton"));
	close_button_->setStyleSheet(qTEXT(R"(
                                         QPushButton#albumViewPageCloseButton {
                                         border: none;
                                         background-color: transparent;
										 border-radius: 0px;
                                         }
										 QPushButton#albumViewPageCloseButton:hover {	
										 border-radius: 0px;
                                         }
                                         )"));
	close_button_->setFixedSize(qTheme.GetTitleButtonIconSize());
	close_button_->setIconSize(qTheme.GetTitleButtonIconSize());
	close_button_->setIcon(qTheme.GetFontIcon(Glyphs::ICON_CLOSE_WINDOW));

	(void)QObject::connect(close_button_, &QPushButton::clicked, [this]() {
		hide();
		});

	auto* hbox_layout = new QHBoxLayout();
	hbox_layout->setSpacing(0);
	hbox_layout->setContentsMargins(15, 15, 0, 0);

	auto* button_spacer = new QSpacerItem(20, 10, QSizePolicy::Expanding, QSizePolicy::Fixed);
	hbox_layout->addWidget(close_button_);
	hbox_layout->addSpacerItem(button_spacer);

	auto* default_layout = new QVBoxLayout(this);
	default_layout->setSpacing(0);
	default_layout->setContentsMargins(0, 5, 0, 0);	

	artist_image_ = new QLabel(this);
	artist_image_->setObjectName(QString::fromUtf8("artistImage"));
	artist_image_->setFixedHeight(300);
	artist_image_->setAlignment(Qt::AlignCenter);
	hbox_layout->addWidget(artist_image_);
	hbox_layout->setStretch(2, 1);

	album_view_ = new AlbumView(this);
	album_view_->setObjectName(QString::fromUtf8("albumView"));
	default_layout->addLayout(hbox_layout);	
	default_layout->addWidget(album_view_);
	default_layout->setStretch(1, 1);

	setStyleSheet(qSTR(
		R"(
              QFrame#artistView {
					background: qlineargradient(
						x1:0, y1:0, x2:1, y2:0, stop:0 #f46b45, stop:1 #eea849
					);
              }
			  QFrame#artistView:height {
					background: qlineargradient(
						x1:0, y1:0, x2:1, y2:0, stop:0 #f46b45, stop:1 #eea849, spread: repeat-y
					);
              }
        )"
	));
}

void ArtistViewPage::resizeEvent(QResizeEvent* event) {
	setProperty("height", rect().height());
}

void ArtistViewPage::SetArtistImage(const QPixmap& image) {
	artist_image_->setPixmap(image);
}

ArtistView::ArtistView(QWidget* parent)
	: QListView(parent)
	, page_(new ArtistViewPage(this))
	, model_(this) {
	setModel(&model_);
	setUniformItemSizes(true);
	setDragEnabled(false);
	setSelectionRectVisible(false);
	setEditTriggers(QAbstractItemView::NoEditTriggers);
	setWrapping(true);
	setFlow(QListView::LeftToRight);
	setViewMode(QListView::IconMode);
	setResizeMode(QListView::Adjust);
	setFrameStyle(QFrame::StyledPanel);
	setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
	setVerticalScrollMode(QAbstractItemView::ScrollPerItem);
	setAutoScroll(false);
	viewport()->setAttribute(Qt::WA_StaticContents);
	setMouseTracking(true);
	auto* styled_delegate = new ArtistStyledItemDelegate(this);
	setItemDelegate(styled_delegate);
	verticalScrollBar()->setStyleSheet(qTEXT(
		"QScrollBar:vertical { width: 6px; }"
	));
	Refresh();
	page_->hide();

	(void)QObject::connect(styled_delegate, &ArtistStyledItemDelegate::EnterAlbumView, [this](auto index) {
		const auto list_view_rect = this->rect();
		auto artistCoverId = GetIndexValue(index, 2).toString();
		auto artist_cover = qAlbumCoverCache.GetCover(artistCoverId);
		page_->SetArtistImage(artist_cover);
		page_->setFixedSize(QSize(list_view_rect.size().width() - 15, list_view_rect.height() + 25));
		page_->move(QPoint(list_view_rect.x() + 3, 0));
		page_->show();
		});
}

void ArtistView::resizeEvent(QResizeEvent* event) {
	if (page_ != nullptr) {
		if (!page_->isHidden()) {
			const auto list_view_rect = this->rect();
			page_->setFixedSize(QSize(list_view_rect.size().width() - 15, list_view_rect.height() + 15));
			page_->move(QPoint(list_view_rect.x() + 3, 0));
		}
	}
	QListView::resizeEvent(event);
}

void ArtistView::OnThemeChanged(QColor backgroundColor, QColor color) {
	dynamic_cast<ArtistStyledItemDelegate*>(itemDelegate())->SetTextColor(color);
}

void ArtistView::Refresh() {
	model_.setQuery(qTEXT(R"(
SELECT
    artists.artist,
    artists.artistId,
    artists.coverId,
	artists.firstChar
FROM
    artists
    )"), qDatabase.database());
}