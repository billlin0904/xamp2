#include "thememanager.h"

#include <QScrollBar>
#include <QApplication>

#include <widget/pixmapcache.h>
#include <widget/database.h>
#include <widget/str_utilts.h>
#include <widget/image_utiltis.h>
#include <widget/artistview.h>

static constexpr QSize kArtistImageSize{ 30, 30 };

ArtistViewStyledDelegate::ArtistViewStyledDelegate(QObject* parent)
    : QStyledItemDelegate(parent) {    
}

void ArtistViewStyledDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {    
    if (!index.isValid()) {
        return;
    }

    auto artist = index.model()->data(index.model()->index(index.row(), 1)).toString();
    auto cover_id = index.model()->data(index.model()->index(index.row(), 2)).toString();
    auto discogs_artist_id = index.model()->data(index.model()->index(index.row(), 3)).toString();
    auto first_char = index.model()->data(index.model()->index(index.row(), 4)).toString();

    const QRect image_react(QPoint{ option.rect.left() + option.rect.width() / 2, option.rect.top() }, kArtistImageSize);

    painter->setRenderHints(QPainter::Antialiasing, true);
    painter->setRenderHints(QPainter::SmoothPixmapTransform, true);

    QPainterPath path;
    path.addEllipse(image_react);    
    painter->setClipPath(path);

    auto f = painter->font();
    f.setPointSize(14);
    f.setBold(true);
    painter->setFont(f);
    painter->fillRect(image_react, Qt::gray);
    painter->setPen(Qt::white);
    painter->drawText(image_react, Qt::AlignCenter, first_char);
}

QSize ArtistViewStyledDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    auto result = QStyledItemDelegate::sizeHint(option, index);
    result.setWidth(kArtistImageSize.width() + 20);
    result.setHeight(kArtistImageSize.height() + 10);
    return result;
}

ArtistView::ArtistView(QWidget *parent)
    : QListView(parent) {
    setModel(&model_);

    setUniformItemSizes(true);    
    setDragEnabled(false);
    setFlow(QListView::TopToBottom);    
    setViewMode(QListView::ListMode);
    setResizeMode(QListView::Adjust);
    //setWrapping(true);
    setFrameStyle(QFrame::NoFrame);
    // 不會出現選擇框
    setSelectionRectVisible(false);
    // 不可編輯
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setStyleSheet(Q_UTF8("background-color: transparent"));
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setItemDelegate(new ArtistViewStyledDelegate());
    verticalScrollBar()->setStyleSheet(Q_UTF8("QScrollBar {width:0px;}"));

    (void) QObject::connect(this, &QListView::clicked, [this](auto index) {        
        auto artist_id = index.model()->data(index.model()->index(index.row(), 0)).toInt();
        auto artist = index.model()->data(index.model()->index(index.row(), 1)).toString();
        auto cover_id_id = index.model()->data(index.model()->index(index.row(), 2)).toString();
        auto discogs_artist_id = index.model()->data(index.model()->index(index.row(), 3)).toString();
        auto first_char = index.model()->data(index.model()->index(index.row(), 4)).toString();
        emit clickedArtist(first_char);              
        });    
}

void ArtistView::refreshOnece() {
    QString query = Q_UTF8(R"(
SELECT
	artists.artistId,
	artists.artist,
	artists.coverId,
	artists.discogsArtistId,
    artists.firstChar
FROM
	artists 
GROUP BY
	artists.firstChar 
ORDER BY
	artists.firstChar
    ;)");
    model_.setQuery(query);
}
