#include <QPainter>
#include <QScrollBar>
#include <QSqlError>
#include <QSqlQuery>
#include <QDebug>
#include <QApplication>

#include <base/logger.h>

#include <widget/database.h>

#include "thememanager.h"
#include <widget/str_utilts.h>
#include <widget/image_utiltis.h>
#include <widget/pixmapcache.h>
#include <widget/albumview.h>

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

    QRect album_text_rect(option.rect.left() + 10, option.rect.top() + default_cover_size.height() + 15, option.rect.width() - 30, 15);
    QRect artist_text_rect(option.rect.left() + 10, option.rect.top() + default_cover_size.height() + 35, option.rect.width() - 30, 15);

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

AlbumView::AlbumView(QWidget* parent)
	: QListView(parent)
	, model_(this) {
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
	setItemDelegate(new AlbumViewStyledDelegate());	

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

	(void) QObject::connect(&model_, &QSqlQueryModel::modelReset, []() {
		QApplication::restoreOverrideCursor();
		});
}

void AlbumView::setFilterByArtist(int32_t artist_id) {
	QString s(
		Q_UTF8(R"(
	SELECT
		album,
		albums.coverId,
		artist
	FROM
		albumArtist 
	LEFT JOIN 
		albums ON albums.albumId = albumArtist.albumId
	LEFT JOIN 
		artists ON artists.artistId = albumArtist.artistId
	WHERE
		( albumArtist.artistId = %1)
	)")
	);

	model_.setQuery(s.arg(artist_id));
}

void AlbumView::refreshOnece(bool setWaitCursor) {
	if (setWaitCursor) {
		QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	}	
	model_.setQuery(Q_UTF8(R"(
	SELECT
		album,
		albums.coverId,
		artist
	FROM
		albums,
		artists
	WHERE
		( albums.artistId = artists.artistId )
	)"));	
}
