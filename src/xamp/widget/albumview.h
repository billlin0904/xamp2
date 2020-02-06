//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QListView>
#include <QSqlQueryModel>
#include <QLabel>
#include <QSqlRelationalTableModel>
#include <QStyledItemDelegate>
#include <QTableView>

class AlbumPlayListTableView;

class AlbumViewStyledDelegate : public QStyledItemDelegate {
	Q_OBJECT
public:
	explicit AlbumViewStyledDelegate(QObject* parent = nullptr);

protected:
	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

	QSize sizeHint(const QStyleOptionViewItem& o, const QModelIndex& idx) const override;
};

class AlbumViewPage : public QFrame {
	Q_OBJECT
public:
	explicit AlbumViewPage(QWidget* parent = nullptr);

	void setAlbum(const QString &album);

	void setArtist(const QString& artist);

	void setPlaylistMusic(int32_t album_id);
private:
	QLabel* album_;
	QLabel* artist_;
	AlbumPlayListTableView* playlist_;
};

class AlbumPlayListTableView : public QTableView {
	Q_OBJECT
public:
	explicit AlbumPlayListTableView(QWidget* parent = nullptr);

	void setPlaylistMusic(int32_t album_id);
private:	
	QSqlQueryModel model_;
};

class AlbumView : public QListView {
	Q_OBJECT
public:
	explicit AlbumView(QWidget* parent = nullptr);

public slots:
	void refreshOnece(bool refreshOnece);

	void setFilterByArtist(int32_t artist_id);

	void hideWidget();

private:
	AlbumViewPage* page_;
	QSqlQueryModel model_;
};

