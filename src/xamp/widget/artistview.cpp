#include <QPainter>

#include "thememanager.h"

#include <QScrollBar>
#include <QApplication>

#include <widget/pixmapcache.h>
#include <widget/database.h>
#include <widget/str_utilts.h>
#include <widget/image_utiltis.h>
#include <widget/artistview.h>

constexpr QSize ARTIST_IMAGE_SIZE{ 30, 30 };

ArtistViewStyledDelegate::ArtistViewStyledDelegate(QObject* parent)
    : QStyledItemDelegate(parent) {    
}

void ArtistViewStyledDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    if (!index.isValid()) {
        return;
    }    

    auto artist_id = index.model()->data(index.model()->index(index.row(), 0)).toInt();
    auto artist = index.model()->data(index.model()->index(index.row(), 1)).toString();
    auto cover_id = index.model()->data(index.model()->index(index.row(), 2)).toString();
    auto discogs_artist_id = index.model()->data(index.model()->index(index.row(), 3)).toString();

    const QRect image_react(QPoint{ option.rect.left(), option.rect.top() }, ARTIST_IMAGE_SIZE);

    painter->setRenderHints(QPainter::Antialiasing, true);
    painter->setRenderHints(QPainter::SmoothPixmapTransform, true);

    QPainterPath path;
    path.addEllipse(image_react);    
    painter->setClipPath(path);    

#if 0
    if (!cover_id.isEmpty()) {
        if (auto image = PixmapCache::Instance().find(cover_id)) {
            auto small_cover = Pixmap::resizeImage(*image.value(), ARTIST_IMAGE_SIZE);
            painter->drawPixmap(image_react, small_cover);
            return;
        }
    }
#endif

    auto f = painter->font();
    f.setPointSize(10);
    f.setBold(true);
    painter->setFont(f);
    painter->fillRect(image_react, Qt::gray);
    painter->setPen(Qt::white);
    painter->drawText(image_react, Qt::AlignCenter, artist.mid(0, 1));
}

QSize ArtistViewStyledDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    auto result = QStyledItemDelegate::sizeHint(option, index);
    result.setWidth(ARTIST_IMAGE_SIZE.width() + 20);
    result.setHeight(ARTIST_IMAGE_SIZE.height() + 10);
    return result;
}

ArtistView::ArtistView(QWidget *parent)
    : QListView(parent)
    , manager_(new QNetworkAccessManager(this))
    , client_(manager_, this) {
    setModel(&model_);
    model_.setEditStrategy(QSqlTableModel::OnManualSubmit);
    model_.setTable(Q_UTF8("artists"));
    model_.select();

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
        auto artist = index.model()->data(index.model()->index(index.row(), 1)).toString();
        auto artist_id_index = dynamic_cast<const QSqlRelationalTableModel*>(index.model())->fieldIndex(Q_UTF8("artistId"));
        auto artist_id = index.model()->data(index.model()->index(index.row(), artist_id_index)).toInt();        
        auto discogs_artist_id_index = dynamic_cast<const QSqlRelationalTableModel*>(index.model())->fieldIndex(Q_UTF8("discogsArtistId"));
        auto discogs_artist_id = index.model()->data(index.model()->index(index.row(), discogs_artist_id_index)).toString();
        auto cover_id_index = dynamic_cast<const QSqlRelationalTableModel*>(index.model())->fieldIndex(Q_UTF8("coverId"));
        auto cover_id_id = index.model()->data(index.model()->index(index.row(), cover_id_index)).toString();
        emit clickedArtist(artist_id);
        if (!discogs_artist_id.isEmpty() && cover_id_id.isEmpty()) {
            client_.searchArtistId(artist_id, discogs_artist_id);
        }        
        });

    (void) QObject::connect(&client_, &DiscogsClient::getArtistImageUrl, [this](auto artist_id, auto url) {
        client_.downloadArtistImage(artist_id, url);
        XAMP_LOG_DEBUG("Download artist id: {}, discogs image url: {}", artist_id, url.toStdString());
        });

    (void) QObject::connect(&client_, &DiscogsClient::downloadImageFinished, [this](auto artist_id, auto image) {
        auto cover_id = PixmapCache::Instance().emplace(image);
        Database::Instance().updateArtistCoverId(artist_id, cover_id);
        XAMP_LOG_DEBUG("Save artist id: {} image, cover id : {}", artist_id, cover_id.toStdString());
        refreshOnece();
        });
}

void ArtistView::refreshOnece() {
    model_.select();
}
