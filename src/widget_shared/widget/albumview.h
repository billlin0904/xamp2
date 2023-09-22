//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QListView>
#include <QPropertyAnimation>
#include <QSqlQueryModel>
#include <QStyledItemDelegate>
#include <QProgressDialog>
#include <QPushButton>

#include <widget/str_utilts.h>
#include <widget/widget_shared.h>
#include <widget/imagecache.h>
#include <widget/themecolor.h>
#include <widget/playlistentity.h>
#include <widget/lazyloadingmodel.h>
#include <widget/widget_shared_global.h>

class XProgressDialog;
class DatabaseFacade;
class QLabel;
class ClickableLabel;
class AlbumViewPage;
class AlbumPlayListTableView;
class PlaylistPage;
class XMessage;

enum ShowModes {
	SHOW_ARTIST,
	SHOW_YEAR,
	SHOW_NORMAL,
};

class AlbumViewStyledDelegate final : public QStyledItemDelegate {
	Q_OBJECT
public:	
	static const ConstLatin1String kAlbumCacheTag;
	static constexpr auto kMoreIconSize = 20;
	static constexpr auto kIconSize = 48;

	explicit AlbumViewStyledDelegate(QObject* parent = nullptr);

	void SetAlbumTextColor(QColor color);

	void EnableAlbumView(bool enable);

	void SetPlayingAlbumId(int32_t album_id) {
		playing_album_id_ = album_id;
	}

	void SetShowMode(ShowModes mode) {
		show_mode_ = mode;
	}

	ShowModes GetShowModes() const {
		return show_mode_;
	}	
signals:
	void EnterAlbumView(const QModelIndex& index) const;

	void ShowAlbumMenu(const QModelIndex& index, const QPoint &pt) const;

protected:
	bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) override;

	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

	QSize sizeHint(const QStyleOptionViewItem& o, const QModelIndex& idx) const override;

private:	
	bool enable_album_view_{ true };
	ShowModes show_mode_{ SHOW_ARTIST };
	int32_t playing_album_id_{ -1 };
	QColor album_text_color_;
	QPoint mouse_point_;
	QPixmap mask_image_;
	QScopedPointer<QPushButton> more_album_opt_button_;
	QScopedPointer<QPushButton> play_button_;
};

class AlbumViewPage final : public QFrame {
	Q_OBJECT
public:
	explicit AlbumViewPage(QWidget* parent = nullptr);

	void SetPlaylistMusic(const QString &album, int32_t album_id, const QString& cover_id, int32_t album_heart);

	PlaylistPage* playlistPage() const {
		return page_;
	}

signals:
	void ClickedArtist(const QString& artist, const QString& cover_id, int32_t artist_id);

	void LeaveAlbumView() const;

public slots:
	void OnCurrentThemeChanged(ThemeColor theme_color);

private:
	void paintEvent(QPaintEvent* event) override;

	PlaylistPage* page_;
	QPushButton* close_button_;	
};

class XAMP_WIDGET_SHARED_EXPORT AlbumView : public QListView {
	Q_OBJECT
public:
    static constexpr auto kPageAnimationDurationMs = 200;
	
	explicit AlbumView(QWidget* parent = nullptr);

	void Update();

	AlbumViewPage* albumViewPage();

	void EnablePage(bool enable);

	void ReadSingleFileTrackInfo(const QString& file_name);

	virtual void ShowAll();

	void SetPlayingAlbumId(int32_t album_id);

	void FilterCategories(const QString& category);

	void FilterCategories(const QSet<QString>& category);

	void FilterYears(const QSet<QString>& years);

	void SortYears();

	void SetShowMode(ShowModes mode);

	void resizeEvent(QResizeEvent* event) override;

signals:
    void AddPlaylist(const QList<int32_t> &music_ids, const QList<PlayListEntity> &entities);

	void ClickedArtist(const QString& artist, const QString& cover_id, int32_t artist_id);

	void ClickedAlbum(const QString& album, int32_t album_id, const QString& cover_id);

	void RemoveAll();

	void LoadCompleted(int32_t total_album, int32_t total_tracks);

	void ExecuteDatabase(const QSharedPointer<DatabaseFacade>& facade,
		QString const& file_path,
		int32_t playlist_id,
		bool is_podcast_mode);

	void ExtractFile(const QString& file_path, int32_t playlist_id);
public slots:
	void OnCurrentThemeChanged(ThemeColor theme_color);

	void Refresh();

	void FilterByArtistId(int32_t artist_id);

	void HideWidget();

	void OnSearchTextChanged(const QString& text);

    void OnThemeChanged(QColor backgroundColor, QColor color);

    void append(const QString& file_name);

	void ShowMenu(const QPoint& pt);

	void ShowAlbumViewMenu(const QPoint& pt);

private:	
	void ShowPageAnimation();

	void HidePageAnimation();

	bool enable_page_{ true };
	bool hide_page_{ false };

protected:
	QString last_query_;

private:
	AlbumViewPage* page_;
	AlbumViewStyledDelegate* styled_delegate_;
	QPropertyAnimation* animation_;
	LazyLoadingModel model_;
};

