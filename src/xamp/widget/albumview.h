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

#include <widget/clickablelabel.h>
#include <widget/str_utilts.h>
#include <widget/playlistentity.h>

class AlbumPlayListTableView;

class AlbumViewStyledDelegate : public QStyledItemDelegate {
	Q_OBJECT
public:
	explicit AlbumViewStyledDelegate(QObject* parent = nullptr);

	void setTextColor(QColor color);

protected:
	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

	QSize sizeHint(const QStyleOptionViewItem& o, const QModelIndex& idx) const override;
private:
	QColor text_color_;
};

class AlbumViewPage : public QFrame {
	Q_OBJECT
public:
	explicit AlbumViewPage(QWidget* parent = nullptr);

	void setAlbum(const QString &album);

	void setArtist(const QString& artist);

	void setArtistId(int32_t artist_id);

	void setPlaylistMusic(int32_t album_id);

	void setCover(const QString &cover_id);

	void setTracks(int32_t tracks);

	void setTotalDuration(double durations);

	void setArtistCover(const QString& cover_id);

	ClickableLabel* artist() {
		return artist_;
	}

	AlbumPlayListTableView* playlist() {
		return playlist_;
	}

signals:
    void playMusic(const MusicEntity& entity);

	void clickedArtist(const QString& artist, const QString& cover_id, int32_t artist_id);

private:
	int32_t artist_id_;
	QString artist_cover_id_;
	QLabel* album_;
	ClickableLabel* artist_;
	QLabel* cover_;
	QLabel* tracks_;
	QLabel* durtions_;
	AlbumPlayListTableView* playlist_;
};

class AlbumPlayListTableView : public QTableView {
	Q_OBJECT
public:
	enum {
		PLAYLIST_TRACK,
		PLAYLIST_TITLE,
		PLAYLIST_DURATION,
	};

	explicit AlbumPlayListTableView(QWidget* parent = nullptr);

	void setPlaylistMusic(int32_t album_id);

private:
	void resizeColumn();

	QSqlQueryModel model_;
};

class AlbumView : public QListView {
	Q_OBJECT
public:
	explicit AlbumView(QWidget* parent = nullptr);

signals:
    void playMusic(const MusicEntity& entity);

    void addPlaylist(const PlayListEntity &entity);

	void clickedArtist(const QString& artist, const QString& cover_id, int32_t artist_id);

public slots:
	void refreshOnece();

	void setFilterByArtistFirstChar(const QString& first_char);

	void setFilterByArtistId(int32_t artist_id);

	void hideWidget();

	void onSearchTextChanged(const QString& text);

	void payNextMusic();

	void onTextColorChanged(QColor backgroundColor, QColor color);

private:
	AlbumViewPage* page_;
	QSqlQueryModel model_;
};

