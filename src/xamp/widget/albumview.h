//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QListView>
#include <QSqlQueryModel>
#include <QSqlRelationalTableModel>
#include <QStyledItemDelegate>

#include "pixmapcache.h"

class AlbumViewStyledDelegate : public QStyledItemDelegate {
	Q_OBJECT
public:
	explicit AlbumViewStyledDelegate(QObject* parent = nullptr);

protected:
	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

	QSize sizeHint(const QStyleOptionViewItem& o, const QModelIndex& idx) const override;

private:
	mutable LruCache<QString, QPixmap> cache_;
};

class AlbumView : public QListView {
	Q_OBJECT
public:
	explicit AlbumView(QWidget* parent = nullptr);

public slots:
	void refreshOnece();

	void setFilterByArtist(int32_t artist_id);

private:
	QSqlQueryModel model_;
};

