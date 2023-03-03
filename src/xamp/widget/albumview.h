//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QListView>
#include <QPropertyAnimation>
#include <QSqlQueryModel>
#include <QStyledItemDelegate>
#include <QPushButton>

#include <widget/widget_shared.h>

#include <widget/themecolor.h>
#include <widget/playlistentity.h>

class XProgressDialog;
class DatabaseFacade;
class QLabel;
class ClickableLabel;
class AlbumViewPage;
class AlbumPlayListTableView;
class PlaylistPage;

class AlbumViewStyledDelegate final : public QStyledItemDelegate {
	Q_OBJECT
public:
	static constexpr size_t kMaxAlbumRoundedImageCacheSize = 100;

	explicit AlbumViewStyledDelegate(QObject* parent = nullptr);

	void SetTextColor(QColor color);

	void ClearImageCache();

	void EnableAlbumView(bool enable);

	void SetPlayingAlbumId(int32_t album_id) {
		playing_album_id_ = album_id;
	}
signals:
	void EnterAlbumView(const QModelIndex& index) const;

	void ShowAlbumMenu(const QModelIndex& index, const QPoint &pt) const;

protected:
	bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) override;

	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

	QSize sizeHint(const QStyleOptionViewItem& o, const QModelIndex& idx) const override;

private:
	QPixmap GetCover(const QString &cover_id) const;

	bool enable_album_view_{ true };
	int32_t playing_album_id_{ -1 };
	QColor text_color_;
	QPoint mouse_point_;
	QPixmap mask_image_;
	QScopedPointer<QPushButton> more_album_opt_button_;
	QScopedPointer<QPushButton> play_button_;
	mutable LruCache<QString, QPixmap> image_cache_;
};

class AlbumViewPage final : public QFrame {
	Q_OBJECT
public:
	explicit AlbumViewPage(QWidget* parent = nullptr);

	void SetPlaylistMusic(const QString &album, int32_t album_id, const QString& cover_id);

	ClickableLabel* artist() {
		return artist_;
	}

	PlaylistPage* playlistPage() {
		return page_;
	}

signals:
	void ClickedArtist(const QString& artist, const QString& cover_id, int32_t artist_id);

	void LeaveAlbumView() const;

public slots:
	void OnCurrentThemeChanged(ThemeColor theme_color);

private:
	ClickableLabel* artist_;
	PlaylistPage* page_;
	QPushButton* close_button_;
};

class AlbumView final : public QListView {
	Q_OBJECT
public:
    static constexpr auto kPageAnimationDurationMs = 200;

	explicit AlbumView(QWidget* parent = nullptr);

	void update();

	AlbumViewPage* albumViewPage() {
		return page_;
	}

	void EnablePage(bool enable);

	void ReadSingleFileTrackInfo(const QString& file_name);

	void SetPlayingAlbumId(int32_t album_id);

signals:
    void AddPlaylist(const ForwardList<int32_t> &music_ids, const ForwardList<PlayListEntity> &entities);

	void ClickedArtist(const QString& artist, const QString& cover_id, int32_t artist_id);

	void ClickedAlbum(const QString& album, int32_t album_id, const QString& cover_id);

	void RemoveAll();

	void LoadCompleted();

	void ReadTrackInfo(const QSharedPointer<DatabaseFacade>& adapter,
		QString const& file_path, 
		int32_t playlist_id,
		bool is_podcast_mode);

public slots:
	void OnCurrentThemeChanged(ThemeColor theme_color);

	void Refresh();

	void SetFilterByArtistId(int32_t artist_id);

	void HideWidget();

	void OnSearchTextChanged(const QString& text);

    void OnThemeChanged(QColor backgroundColor, QColor color);

    void append(const QString& file_name);

    void ProcessTrackInfo(const ForwardList<TrackInfo> & entities);

	void ShowMenu(const QPoint& pt);

	void ShowAlbumViewMenu(const QPoint& pt);

	void OnReadFileStart(int dir_size);

	void OnReadFileProgress(const QString& dir, int progress);

	void OnReadFileEnd();
private:
	void resizeEvent(QResizeEvent* event) override;

	void ShowPageAnimation();

	void HidePageAnimation();

	bool enable_page_{ true };
	bool hide_page_{ false };
	AlbumViewPage* page_;
	AlbumViewStyledDelegate* styled_delegate_;
	QPropertyAnimation* animation_;
	QSqlQueryModel model_;
	QSharedPointer<XProgressDialog> read_progress_dialog_;
};

