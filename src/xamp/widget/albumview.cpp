#include <QPainter>
#include <QScrollBar>

#include "thememanager.h"
#include "str_utilts.h"
#include "image_utiltis.h"
#include "pixmapcache.h"
#include "albumview.h"

AlbumViewStyledDelegate::AlbumViewStyledDelegate(QObject* parent)
	: QStyledItemDelegate(parent)
	, cache_(65535) {
	cache_unknown_cover_ = Pixmap::resizeImage(ThemeManager::pixmap().unknownCover(), ThemeManager::getDefaultCoverSize());
}

void AlbumViewStyledDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
	if (!index.isValid()) {
		return;
	}

	auto album = index.model()->data(index.model()->index(index.row(), 2)).toString();
	auto coverId = index.model()->data(index.model()->index(index.row(), 3)).toString();

	auto artistIndex = dynamic_cast<const QSqlRelationalTableModel*>(index.model())->fieldIndex(Q_UTF8("artist"));
	auto artist = index.model()->data(index.model()->index(index.row(), artistIndex)).toString();

	const auto default_cover = ThemeManager::getDefaultCoverSize();
	const QRect cover_rect(option.rect.left() + 10, option.rect.top() + 10,
		default_cover.width(), default_cover.height());

	QRect albumTextRect(option.rect.left() + 10, option.rect.top() + default_cover.height() + 15, option.rect.width() - 10, 15);
	QRect artistTextRect(option.rect.left() + 10, option.rect.top() + default_cover.height() + 35, option.rect.width() - 10, 15);
	
	const auto textOption = Qt::AlignVCenter;
	auto f = painter->font();
	f.setBold(true);
	painter->setFont(f);
	painter->drawText(albumTextRect, textOption, album);
	f.setBold(false);
	painter->setFont(f);
	painter->drawText(artistTextRect, textOption, artist);

	if (coverId.isEmpty()) {
		painter->drawPixmap(cover_rect, cache_unknown_cover_);
		return;
	}

	if (auto cache_small_cover = cache_.findOrNull(coverId)) {
		painter->drawPixmap(cover_rect, *cache_small_cover.value());
		return;
	}

	if (auto cache = PixmapCache::Instance().find(coverId)) {
		auto small_cover = Pixmap::resizeImage(*cache.value(), default_cover);
		painter->drawPixmap(cover_rect, small_cover);
		cache_.emplace(coverId, std::move(small_cover));
	}
	else {
		painter->drawPixmap(cover_rect, cache_unknown_cover_);
	}
}

QSize AlbumViewStyledDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
	auto result = QStyledItemDelegate::sizeHint(option, index);	
	const auto default_cover = ThemeManager::getDefaultCoverSize();
	result.setWidth(default_cover.width() + 10);
	result.setHeight(default_cover.height() + 50);
	return result;
}

AlbumView::AlbumView(QWidget* parent)
	: QListView(parent)
	, model_(this) {
	setModel(&model_);
	model_.setTable(Q_UTF8("albums"));
	model_.setRelation(1, QSqlRelation(Q_UTF8("artists"), Q_UTF8("artistId"), Q_UTF8("artist")));
	model_.select();
	setUniformItemSizes(true);
	setDragEnabled(false);
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

void AlbumView::updateAlbumCover() {
	model_.select();
}