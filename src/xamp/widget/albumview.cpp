#include <QPainter>
#include <QScrollBar>
#include <QSqlError>
#include <QSqlQuery>
#include <QDebug>
#include <QApplication>
#include <QVBoxLayout>
#include <QPushButton>
#include <QSpacerItem>
#include <QHeaderView>

#include <base/logger.h>

#include <widget/database.h>
#include <widget/playlisttableview.h>

#include "thememanager.h"
#include <widget/str_utilts.h>
#include <widget/time_utilts.h>
#include <widget/image_utiltis.h>
#include <widget/pixmapcache.h>
#include <widget/albumview.h>

static QString colourToString(QColor colour) {
	QString s = QString::number(colour.red()) + Q_UTF8(",") +
		QString::number(colour.green()) + Q_UTF8(",") +
		QString::number(colour.blue());
	if (colour.alpha() != 255) {
		s += Q_UTF8(",") + QString::number(colour.alpha());
	}
	return s;
}

#define MAKE_TEXT_COLOR(color) \
	QString(Q_UTF8("color: rgb(%1);")).arg(colourToString(color)) \

#define MAKE_BK_COLOR(color) \
	QString(Q_UTF8("background-color: rgb(%1);")).arg(colourToString(color)) \

#define MAKE_COLOR(color) \
	QString(Q_UTF8("color: rgb(%1);")).arg(colourToString(color)) \

AlbumViewStyledDelegate::AlbumViewStyledDelegate(QObject* parent)
	: QStyledItemDelegate(parent) {
}

void AlbumViewStyledDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
	if (!index.isValid()) {
		return;
	}

	painter->setRenderHints(QPainter::Antialiasing, true);
	painter->setRenderHints(QPainter::SmoothPixmapTransform, true);

	auto album = index.model()->data(index.model()->index(index.row(), 0)).toString();
	auto cover_id = index.model()->data(index.model()->index(index.row(), 1)).toString();
	auto artist = index.model()->data(index.model()->index(index.row(), 2)).toString();

	const auto default_cover_size = ThemeManager::getDefaultCoverSize();
	const QRect cover_rect(option.rect.left() + 10, option.rect.top() + 10,
		default_cover_size.width(), default_cover_size.height());

	QRect album_text_rect(option.rect.left() + 10,
		option.rect.top() + default_cover_size.height() + 15,
		option.rect.width() - 30, 15);
	QRect artist_text_rect(option.rect.left() + 10,
		option.rect.top() + default_cover_size.height() + 35,
		option.rect.width() - 30, 15);

	auto f = painter->font();
	f.setPointSize(9);

	f.setBold(true);
	painter->setFont(f);
	painter->drawText(album_text_rect, Qt::AlignVCenter, album);

	f.setBold(false);
	painter->setFont(f);
	painter->drawText(artist_text_rect, Qt::AlignVCenter, artist);

	auto album_cover = &ThemeManager::pixmap().defaultSizeUnknownCover();

	if (auto cache_small_cover = PixmapCache::Instance().find(cover_id)) {
		album_cover = cache_small_cover.value();
		painter->drawPixmap(cover_rect, *album_cover);
	}
	else {
		painter->drawPixmap(cover_rect, *album_cover);
	}
}

QSize AlbumViewStyledDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
	auto result = QStyledItemDelegate::sizeHint(option, index);
	const auto default_cover = ThemeManager::getDefaultCoverSize();
	result.setWidth(default_cover.width() + 30);
	result.setHeight(default_cover.height() + 70);
	return result;
}

AlbumPlayListTableView::AlbumPlayListTableView(QWidget* parent)
	: QTableView(parent) {
	setModel(&model_);

	auto f = font();
#ifdef Q_OS_WIN
	f.setPointSize(9);
#else
	f.setPointSize(12);
#endif
	setFont(f);

	setUpdatesEnabled(true);
	setAcceptDrops(true);
	setDragEnabled(true);
	setShowGrid(false);

	setDragDropMode(InternalMove);
	setFrameShape(NoFrame);
	setFocusPolicy(Qt::NoFocus);

	setHorizontalScrollMode(ScrollPerPixel);
	setVerticalScrollMode(ScrollPerPixel);
	setSelectionMode(ExtendedSelection);
	setSelectionBehavior(SelectRows);

	verticalHeader()->setVisible(false);

	setColumnWidth(0, 5);
	horizontalHeader()->hide();
	horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

	verticalScrollBar()->setStyleSheet(Q_UTF8(R"(
    QScrollBar:vertical {
        background: #FFFFFF;
		width: 9px;
    }
	QScrollBar::handle:vertical {
		background: #dbdbdb;
		border-radius: 3px;
		min-height: 20px;
		border: none;
	}
	QScrollBar::handle:vertical:hover {
		background: #d0d0d0;
	}
	)"));

	setStyleSheet(Q_UTF8("background-color: transparent"));
}

void AlbumPlayListTableView::resizeColumn() {
	auto header = horizontalHeader();

	for (auto column = 0; column < header->count(); ++column) {
		switch (column) {
		case AlbumPlayListTableView::PLAYLIST_TRACK:
			header->setSectionResizeMode(column, QHeaderView::ResizeToContents);
			header->resizeSection(column, 12);
			break;
		case AlbumPlayListTableView::PLAYLIST_TITLE:
			header->setSectionResizeMode(column, QHeaderView::ResizeToContents);
			header->resizeSection(column, 450);
			break;
		case AlbumPlayListTableView::PLAYLIST_DURATION:
			header->setSectionResizeMode(column, QHeaderView::Fixed);
			header->resizeSection(column, 60);
			break;
		default:
			header->setSectionResizeMode(column, QHeaderView::Stretch);
			break;
		}
	}
}

void AlbumPlayListTableView::setPlaylistMusic(int32_t album_id) {
	QString query = Q_UTF8(R"(
SELECT
	musics.track,
	musics.title,
	musics.durationStr,
	musics.musicId,
	artists.artist,
	musics.fileExt,
	musics.path,
	albums.coverId,
	albums.album
FROM
	albumMusic
	JOIN albums ON albums.albumId = albumMusic.albumId
	JOIN artists ON artists.artistId = albumMusic.artistId
	JOIN musics ON musics.musicId = albumMusic.musicId 
WHERE
	albums.albumId = %1;)");
	model_.setQuery(query.arg(album_id));
	setColumnHidden(3, true);
	setColumnHidden(4, true);
	setColumnHidden(5, true);
	setColumnHidden(6, true);
	setColumnHidden(7, true);
	setColumnHidden(8, true);
	resizeColumn();
}

AlbumViewPage::AlbumViewPage(QWidget* parent)
	: QFrame(parent) {
	setStyleSheet(Q_UTF8("background: white;"));
	setFrameStyle(QFrame::NoFrame);

	auto default_layout = new QVBoxLayout(this);
	default_layout->setSpacing(0);
	default_layout->setContentsMargins(10, 10, 10, 0);

	auto close_button = new QPushButton(tr("X"), this);
	close_button->setFixedSize(QSize(24, 24));
	close_button->setStyleSheet(Q_UTF8("border: none"));

	auto hbox_layout = new QHBoxLayout();
	hbox_layout->setSpacing(0);
	hbox_layout->setContentsMargins(0, 0, 0, 0);

	auto f = font();

	f.setPixelSize(18);
	f.setBold(true);
	album_ = new QLabel(this);	
	album_->setMaximumSize(QSize(16777215, 32));
	album_->setFont(f);

	auto artist_layout = new QHBoxLayout();
	artist_layout->setSpacing(0);
	artist_layout->setContentsMargins(0, 0, 0, 0);

	f.setPixelSize(14);
	f.setBold(false);	
	artist_ = new QLabel(this);
	artist_->setMaximumSize(QSize(400, 32));
	artist_->setFont(f);
	artist_->setAlignment(Qt::AlignLeft);

	tracks_ = new QLabel(this);
	tracks_->setFixedSize(QSize(16777215, 64));
	tracks_->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
	tracks_->setFont(f);

	durtions_ = new QLabel(this);
	durtions_->setFixedSize(QSize(16777215, 64));
	durtions_->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
	durtions_->setFont(f);

	artist_layout->addWidget(artist_);
	artist_layout->addWidget(tracks_);
	artist_layout->addWidget(durtions_);
	artist_layout->setStretch(0, 0);
	artist_layout->setStretch(1, 2);
	artist_layout->setStretch(2, 2);
	
	auto button_spacer = new QSpacerItem(20, 10, QSizePolicy::Expanding, QSizePolicy::Expanding);
	hbox_layout->addWidget(close_button);
	hbox_layout->addSpacerItem(button_spacer);	

	default_layout->addLayout(hbox_layout);
	default_layout->addWidget(album_);
	default_layout->addLayout(artist_layout);

	auto playlist_layout = new QHBoxLayout();
	playlist_ = new AlbumPlayListTableView(this);
	cover_ = new QLabel(this);
	cover_->setMinimumSize(QSize(325, 325));
	cover_->setMaximumSize(QSize(325, 325));
	playlist_layout->addWidget(playlist_);
	playlist_layout->addWidget(cover_);
	default_layout->setStretch(2, 2);
	
	default_layout->addLayout(playlist_layout);
	default_layout->setStretch(1, 0);
	default_layout->setStretch(3, 2);

	(void)QObject::connect(close_button, &QPushButton::clicked, [this]() {
		hide();
		});

	setStyleSheet(Q_UTF8("background-color: white"));

	(void)QObject::connect(playlist_, &QTableView::doubleClicked, [this](const QModelIndex& index) {
		auto title = index.model()->data(index.model()->index(index.row(), 1)).toString();
		auto musicId = index.model()->data(index.model()->index(index.row(), 3)).toInt();
		auto artist = index.model()->data(index.model()->index(index.row(), 4)).toString();
		auto file_ext = index.model()->data(index.model()->index(index.row(), 5)).toString();
		auto file_path = index.model()->data(index.model()->index(index.row(), 6)).toString();
		auto cover_id = index.model()->data(index.model()->index(index.row(), 7)).toString();
		auto album = index.model()->data(index.model()->index(index.row(), 8)).toString();
		emit playMusic(album, title, artist, file_path, file_ext, cover_id);
		});
}

void AlbumViewPage::setAlbum(const QString& album) {
	album_->setText(album);
}

void AlbumViewPage::setArtist(const QString& artist) {
	artist_->setText(artist);
}

void AlbumViewPage::setPlaylistMusic(int32_t album_id) {
	playlist_->setPlaylistMusic(album_id);
}

void AlbumViewPage::setTracks(int32_t tracks) {
	tracks_->setText(tr("%1 Songs").arg(tracks));
}

void AlbumViewPage::setTotalDuration(double durations) {
	durtions_->setText(Time::msToString(durations));
}

void AlbumViewPage::setCover(const QString& cover_id) {
	constexpr QSize image_size(250, 250);
	if (auto cache_small_cover = PixmapCache::Instance().find(cover_id)) {
		cover_->setPixmap(Pixmap::resizeImage(cache_small_cover.value()->copy(), image_size));
	}
	else {
		setStyleSheet(Q_UTF8("background-color: white"));
		cover_->setPixmap(Pixmap::resizeImage(ThemeManager::pixmap().defaultSizeUnknownCover(), image_size));
	}
}

AlbumView::AlbumView(QWidget* parent)
	: QListView(parent)
	, model_(this)
	, page_(nullptr) {
	setModel(&model_);
	refreshOnece(false);
	setUniformItemSizes(true);
	setDragEnabled(false);
	// 不會出現選擇框
	setSelectionRectVisible(false);
	// 不可編輯
	setEditTriggers(QAbstractItemView::NoEditTriggers);
	setWrapping(true);
	setFlow(QListView::LeftToRight);
	setViewMode(QListView::IconMode);
	setResizeMode(QListView::Adjust);
	setFrameStyle(QFrame::NoFrame);
	setStyleSheet(Q_UTF8("background-color: transparent"));
	setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
	setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	setItemDelegate(new AlbumViewStyledDelegate(this));

	verticalScrollBar()->setStyleSheet(Q_UTF8(R"(
    QScrollBar:vertical {
        background: #FFFFFF;
		width: 9px;
    }
	QScrollBar::handle:vertical {
		background: #dbdbdb;
		border-radius: 3px;
		min-height: 20px;
		border: none;
	}
	QScrollBar::handle:vertical:hover {
		background: #d0d0d0;
	}
	)"));

	page_ = new AlbumViewPage(this);

	(void)QObject::connect(page_, &AlbumViewPage::playMusic, this, &AlbumView::playMusic);

	(void)QObject::connect(this, &QListView::clicked, [this](auto index) {
		auto album = index.model()->data(index.model()->index(index.row(), 0)).toString();
		auto cover_id = index.model()->data(index.model()->index(index.row(), 1)).toString();
		auto artist = index.model()->data(index.model()->index(index.row(), 2)).toString();
		auto album_id = index.model()->data(index.model()->index(index.row(), 3)).toInt();

		constexpr auto height = 460;

		auto list_view_rect = this->rect();
		auto rect = visualRect(index);

		page_->setAlbum(album);
		page_->setArtist(artist);
		page_->setCover(cover_id);
		page_->setPlaylistMusic(album_id);
		page_->setFixedSize(QSize(list_view_rect.size().width() - 10, height));
		if (auto album_stats = Database::Instance().getAlbumStats(album_id)) {
			page_->setTracks(album_stats.value().tracks);
			page_->setTotalDuration(album_stats.value().durations);
		}		
		
		if (rect.y() + height > list_view_rect.height()) {
			page_->move(QPoint(list_view_rect.x(), rect.y() - height));
		}
		else {
			page_->move(QPoint(list_view_rect.x(), rect.y() + 200));
		}

		page_->show();
		});

	(void)QObject::connect(verticalScrollBar(), &QScrollBar::valueChanged, [this](auto value) {
		hideWidget();
		});
}

void AlbumView::setFilterByArtist(int32_t artist_id) {
	QString s(
	Q_UTF8(R"(
	SELECT
		album,
		albums.coverId,
		artist,
		albums.albumId
	FROM
		albumArtist 
	LEFT JOIN 
		albums ON albums.albumId = albumArtist.albumId
	LEFT JOIN 
		artists ON artists.artistId = albumArtist.artistId
	WHERE
		( albumArtist.artistId = %1 )
	)")
	);

	model_.setQuery(s.arg(artist_id));
	hideWidget();
}

void AlbumView::refreshOnece(bool setWaitCursor) {
	model_.setQuery(Q_UTF8(R"(
	SELECT
		album,
		albums.coverId,
		artist,
		albums.albumId
	FROM
		albums,
		artists
	WHERE
		( albums.artistId = artists.artistId )
	)"));

	hideWidget();
}

void AlbumView::hideWidget() {
	if (!page_) {
		return;
	}
	page_->hide();
}