//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFrame>
#include <widget/widget_shared_global.h>
#include <widget/tabpage.h>
#include <widget/dao/artistdao.h>

class QPushButton;
class QListWidget;
class PlaylistTableView;
class AlbumView;
class QLabel;

class SongItem : public QWidget {
    Q_OBJECT
public:
    explicit SongItem(const QPixmap& album_cover,
        const QString& song_title,
        const QString& artist,
        const QString& album_title,
		const QString& video_id,
        QWidget* parent = nullptr);

    QString videoId() const {
        return video_id_;
    }

    QString artist() const {
        return artist_;
    }

    QString albumTitle() const;

	QString songTitle() const;
private:
    QLabel* cover_label_;
    QLabel* song_label_;
    QLabel* album_label_;
	QString artist_;
    QString video_id_;
};

class AlbumItem : public QWidget {
    Q_OBJECT
public:
    explicit AlbumItem(const QPixmap& cover,
        const QString& album_title,
        const QString& year,
        const QString& video_id,
        QWidget* parent = nullptr);

    QString videoId() const {
        return video_id_;
	}

    QString albumTitle() const;

    void setCover(const QPixmap& cover);

    QPixmap cover() const;
private:
    QLabel* cover_label_;
    QLabel* album_title_label_;
    QLabel* year_label_;
    QString video_id_;
    QPixmap cover_;
};

class XAMP_WIDGET_SHARED_EXPORT ArtistInfoPage : public QFrame, public TabPage {
    Q_OBJECT
public:
    explicit ArtistInfoPage(QWidget* parent = nullptr);

    void setTitle(const QString &title);

    void setDescription(const QString& info);

    void setArtistImage(const QPixmap &image);

    void setArtistId(int32_t artist_id);

    void insertSong(const QString &album, 
        const QString& title,
        const QString& artist,
        const QString& video_id,
        const QPixmap &image = QPixmap());

    void insertAlbum(const QString& album_title,
        const QString& year,
        const QString& video_id,
        const QPixmap& image = QPixmap());

    QListWidget* songList() {
        return song_list_;
    }

    QListWidget* albumList() {
        return album_list_;
    }

    void reload() override;

signals:
    void browseAlbum(const QPixmap &album_cover, const QString& video_id);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    bool is_expanded_{ false };
    int32_t artist_id_;
    QWidget* top_widget_;
    QLabel* title_label_;
    QLabel* desc_label_;
    QListWidget* song_list_;
    QListWidget* album_list_;
    QPushButton* toggle_btn_;
    QString full_description_;
    QString short_description_;
    QPixmap background_;
};