#include <QPainter>

#include "thememanager.h"

#include <widget/pixmapcache.h>
#include <widget/database.h>
#include <widget/str_utilts.h>
#include <widget/artistview.h>

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
    auto artist_image_url = index.model()->data(index.model()->index(index.row(), 3)).toString();

    /*
    QPainterPath path;
    path.addEllipse(image_react);
    painter->setClipPath(path);
    */

    painter->drawText(option.rect, artist);
}

QSize ArtistViewStyledDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    return QStyledItemDelegate::sizeHint(option, index);
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
    setFrameStyle(QFrame::NoFrame);
    setStyleSheet(Q_UTF8("background-color: transparent"));
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setItemDelegate(new ArtistViewStyledDelegate());
}

void ArtistView::refreshOnece() {
    model_.select();
}
