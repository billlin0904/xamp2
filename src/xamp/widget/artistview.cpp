#include <QPainter>

#include "thememanager.h"

#include <QScrollBar>
#include <QApplication>

#include <widget/pixmapcache.h>
#include <widget/database.h>
#include <widget/str_utilts.h>
#include <widget/image_utiltis.h>
#include <widget/artistview.h>

constexpr QSize ARTIST_IMAGE_SIZE{ 40, 40 };

ArtistViewStyledDelegate::ArtistViewStyledDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
    , manager_(new QNetworkAccessManager(this))
    , client_(manager_, this) {
    (void) QObject::connect(&client_, &DiscogsClient::getArtistId, [this](auto artist_id, auto id) {
        Database::Instance().updateDiscogsArtistId(artist_id, id);
        client_.searchArtistId(artist_id, id);
        });

    (void)QObject::connect(&client_, &DiscogsClient::getArtistImageUrl, [this](auto artist_id, auto url) {
        client_.downloadArtistImage(artist_id, url);
        });

    (void) QObject::connect(&client_, &DiscogsClient::downloadImageFinished, [this](auto artist_id, auto image) {
        auto cover_id = PixmapCache::Instance().emplace(image);
        Database::Instance().updateArtistCoverId(artist_id, cover_id);
        });    
}

void ArtistViewStyledDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    if (!index.isValid()) {
        return;
    }    

    auto artist_id = index.model()->data(index.model()->index(index.row(), 0)).toInt();
    auto artist = index.model()->data(index.model()->index(index.row(), 1)).toString();
    auto cover_id = index.model()->data(index.model()->index(index.row(), 2)).toString();
    auto discogs_artist_id = index.model()->data(index.model()->index(index.row(), 3)).toString();

    //auto album_id_index = dynamic_cast<const QSqlRelationalTableModel*>(index.model())->fieldIndex(Q_UTF8("albumId"));
    //auto album_id = index.model()->data(index.model()->index(index.row(), album_id_index)).toString();
    
    const QRect image_react(QPoint{ option.rect.left(), option.rect.top() }, ARTIST_IMAGE_SIZE);

    painter->setRenderHints(QPainter::Antialiasing, true);
    painter->setRenderHints(QPainter::SmoothPixmapTransform, true);

    QPainterPath path;
    path.addEllipse(image_react);    
    painter->setClipPath(path);    

    if (!cover_id.isEmpty()) {
        if (auto image = PixmapCache::Instance().find(cover_id)) {
            auto small_cover = Pixmap::resizeImage(*image.value(), ARTIST_IMAGE_SIZE);
            painter->drawPixmap(image_react, small_cover);
            return;
        }
    }

    auto f = painter->font();
    f.setPointSize(14);
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
    : QListView(parent) {
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
    // ���|�X�{��ܮ�
    setSelectionRectVisible(false);
    // ���i�s��
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
        emit clickedArtist(artist_id);
        });
}

void ArtistView::refreshOnece() {
    model_.select();
}