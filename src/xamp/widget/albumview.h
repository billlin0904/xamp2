//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QListView>
#include <QFileSystemModel>
#include <QStyledItemDelegate>

class AlbumViewStyledDelegate : public QStyledItemDelegate {
	Q_OBJECT
public:
	explicit AlbumViewStyledDelegate(QObject* parent = nullptr);

protected:
	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

	QSize sizeHint(const QStyleOptionViewItem& o, const QModelIndex& idx) const override;
};

class AlbumView : public QListView {
	Q_OBJECT
public:
	explicit AlbumView(QWidget* parent = nullptr);

private:
	QFileSystemModel model_;
};

