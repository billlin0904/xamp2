//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QListView>
#include <QSqlQueryModel>
#include <QSqlRelationalTableModel>
#include <QStyledItemDelegate>
#include <QTableView>

#include <widget/widget_shared.h>

#include <widget/albumentity.h>
#include <widget/playlistentity.h>

class QLabel;
class ClickableLabel;
class AlbumViewPage;
class AlbumPlayListTableView;
class PlaylistPage;

class AlbumViewStyledDelegate final : public QStyledItemDelegate {
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

class AlbumViewPage final : public QFrame {
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

	PlaylistPage* playlistPage() {
		return page_;
	}

signals:
    void playMusic(const AlbumEntity& entity);

	void clickedArtist(const QString& artist, const QString& cover_id, int32_t artist_id);

private:
	int32_t artist_id_;
	QString artist_cover_id_;
	QLabel* album_;
	ClickableLabel* artist_;
	QLabel* cover_;
	QLabel* tracks_;
	QLabel* durtions_;
	PlaylistPage* page_;
	AlbumPlayListTableView* playlist_;
};

class AlbumPlayListTableView final : public QTableView {
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

class AlbumView final : public QListView {
	Q_OBJECT
public:
	explicit AlbumView(QWidget* parent = nullptr);

	void update();

	AlbumViewPage* albumViewPage() {
		return page_;
	}

signals:
    void playMusic(const AlbumEntity& entity);

    void addPlaylist(const std::vector<int32_t> &music_ids, const std::vector<PlayListEntity> &entities);

	void clickedArtist(const QString& artist, const QString& cover_id, int32_t artist_id);

	void removeAll();

	void loadCompleted();
public slots:
	void refreshOnece();

	void setFilterByArtistFirstChar(const QString& first_char);

	void setFilterByArtistId(int32_t artist_id);

	void hideWidget();

	void onSearchTextChanged(const QString& text);

	void payNextMusic();

    void onThemeChanged(QColor backgroundColor, QColor color);

    void append(const QString& file_name);

    void processMeatadata(int64_t dir_last_write_time, const ForwardList<Metadata> &medata);
private:
	AlbumViewPage* page_;
	QSqlQueryModel model_;
};

