#include <QPainter>
#include <QScrollBar>

#include <base/logger.h>
#include "thememanager.h"
#include "str_utilts.h"
#include "image_utiltis.h"
#include "pixmapcache.h"
#include "albumview.h"

AlbumViewStyledDelegate::AlbumViewStyledDelegate(QObject* parent)
	: QStyledItemDelegate(parent) {

}

void AlbumViewStyledDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
	if (!index.isValid()) {
		return;
	}

	auto album = index.model()->data(index.model()->index(index.row(), 2)).toString();
    auto cover_id = index.model()->data(index.model()->index(index.row(), 3)).toString();

    auto artist_index = dynamic_cast<const QSqlRelationalTableModel*>(index.model())->fieldIndex(Q_UTF8("artist"));
    auto artist = index.model()->data(index.model()->index(index.row(), artist_index)).toString();

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

    if (cover_id.isEmpty()) {
        painter->drawPixmap(cover_rect, ThemeManager::pixmap().defaultSizeUnknownCover());
		return;
	}

    if (auto cache_small_cover = cache_.find(cover_id)) {
        painter->drawPixmap(cover_rect, *cache_small_cover.value());
		return;
	}

    auto file_cache = PixmapCache::Instance().fromFileCache(cover_id);
    if (!file_cache.isNull()) {
        auto small_cover = Pixmap::resizeImage(file_cache, default_cover_size);
        //XAMP_LOG_DEBUG("Find in file cover cache id:{}", cover_id.toStdString());
        painter->drawPixmap(cover_rect, small_cover);
        cache_.insert(cover_id, small_cover);
    } else {
        painter->drawPixmap(cover_rect, ThemeManager::pixmap().defaultSizeUnknownCover());
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
	model_.setTable(Q_UTF8("albums"));
	model_.setEditStrategy(QSqlTableModel::OnRowChange);
	model_.setRelation(1, QSqlRelation(Q_UTF8("artists"), Q_UTF8("artistId"), Q_UTF8("artist")));
	model_.select();	

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
}

void AlbumView::filterArtist(int32_t artist_id) {

}

void AlbumView::refreshOnece() {
	model_.select();
}
