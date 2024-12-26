//=====================================================================================================================
// Copyright (c) 2018-2025 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QListView>
#include <QPropertyAnimation>
#include <QStyledItemDelegate>
#include <QProgressDialog>
#include <QPushButton>
#include <QTimer>
#include <QCheckBox>
#include <QSqlQueryModel>

#include <widget/util/str_util.h>
#include <widget/databasecoverid.h>
#include <widget/widget_shared.h>
#include <widget/imagecache.h>
#include <widget/themecolor.h>
#include <widget/playlistentity.h>
#include <widget/widget_shared_global.h>
#include <widget/albumviewstyleddelegate.h>
#include <widget/playlisttableproxymodel.h>

#include <widget/dao/playlistdao.h>
#include <widget/dao/artistdao.h>
#include <widget/dao/albumdao.h>

class XProgressDialog;
class DatabaseFacade;
class QLabel;
class ClickableLabel;
class AlbumViewPage;
class AlbumPlayListTableView;
class PlaylistPage;
class XMessage;
class ProgressView;
class QEnterEvent;

class AlbumViewPage final : public QFrame {
	Q_OBJECT
public:
	explicit AlbumViewPage(QWidget* parent = nullptr);

	void setPlaylistMusic(const QString &album, int32_t album_id, const QString& cover_id, int32_t album_heart);

	PlaylistPage* playlistPage() const {
		return page_;
	}

signals:
	void clickedArtist(const QString& artist, const QString& cover_id, int32_t artist_id);

	void leaveAlbumView() const;

public slots:
	void onThemeChangedFinished(ThemeColor theme_color);

	void onRetranslateUi();

private:
	PlaylistPage* page_;
	QPushButton* close_button_;	
	dao::PlaylistDao playlist_dao_;
	dao::AlbumDao album_dao_;
};

class XAMP_WIDGET_SHARED_EXPORT AlbumView : public QListView {
	Q_OBJECT
public:
    static constexpr auto kPageAnimationDurationMs = 200;

	enum FilterType {
		FILTER_IN,
		FILTER_AND,
		FILTER_OR
	};
	
	explicit AlbumView(QWidget* parent = nullptr);

	AlbumViewPage* albumViewPage();

	void enablePage(bool enable);

	virtual void showAll();

	void setPlayingAlbumId(int32_t album_id);

	void filterCategories(const QString& category);

	void filterCategories(const QSet<QString>& category, FilterType filterType = FILTER_IN);

	void filterYears(const QSet<QString>& years);

	void filterRecentPlays();

	void sortYears();

	void setShowMode(ShowModes mode);

	AlbumViewStyledDelegate* styledDelegate();

	void resizeEvent(QResizeEvent* event) override;

	void refreshCover();
signals:
    void addPlaylist(int32_t playlist_id, const QList<int32_t> &music_ids);

	void clickedArtist(const QString& artist, const QString& cover_id, int32_t artist_id);

	void clickedAlbum(const QString& album, int32_t album_id, const QString& cover_id);

	void removeAll();

	void removeSelectedAlbum(int32_t album_id);

	void extractFile(const QString& file_path, int32_t playlist_id);

public slots:
	void onRetranslateUi();

	void onThemeChangedFinished(ThemeColor theme_color);

	void reload();

	void filterByArtistId(int32_t artist_id);

	void hideWidget();

	void search(const QString& keyword);

    void append(const QString& file_name);

	void showMenu(const QPoint& pt);

	void showAlbumViewMenu(const QPoint& pt);

    void enterEvent(QEnterEvent* event) override;
private:
	bool enable_page_{ true };

protected:
	QString last_query_;

private:
	QTimer refresh_cover_timer_;
	AlbumViewPage* page_;
	QPropertyAnimation* animation_;
	QSqlQueryModel model_;
	PlayListTableFilterProxyModel* proxy_model_;
	dao::PlaylistDao playlist_dao_;
	dao::AlbumDao album_dao_;
	dao::ArtistDao artist_dao_;
};

