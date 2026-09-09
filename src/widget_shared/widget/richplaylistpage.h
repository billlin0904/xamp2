//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFrame>
#include <widget/playbacksnapshot.h>
#include <QPoint>
#include <QPixmap>
#include <QSet>

#include <base/trackinfo.h>

#include <widget/databasecoverid.h>
#include <widget/playlistentity.h>
#include <widget/database.h>
#include <widget/tabpage.h>
#include <widget/widget_shared.h>
#include <widget/widget_shared_global.h>

class QPixmap;
class QLabel;
class QPushButton;
class QModelIndex;
class RichPlaylistCoverPanel;
class RichPlaylistView;
class ScanFileProgressPage;

class XAMP_WIDGET_SHARED_API RichPlaylistPage final : public QFrame, public TabPage {
	Q_OBJECT
public:
	explicit RichPlaylistPage(QWidget* parent = nullptr);

	void reload() override;

	void applyPlayback(const PlaybackSnapshot& state);
	QList<PlayListEntity> tracks() const;

	void onAlbumCoverLoaded(int32_t album_id);

	ScanFileProgressPage* progressPage() const;

	bool playNextItem(int32_t forward);

	void loadPath(const QString& file_path, bool append_to_playlist);

    void setPlaylist(int32_t id, const QString& name);
    void setPlaylistName(const QString& name);
    void search(const QString& text);
    void toggleQueue();
    void refreshPresentation();
    void retranslate();

signals:
    void playOrderChanged();
    void playQueuedTrack(int32_t playlist_music_id);
    void favoriteRequested(bool favorite);
	void playMusic(int32_t playlist_id, const PlayListEntity& item, bool is_play);

	void extractFile(const QString& file_path, int32_t playlist_id);

	void findAlbumCover(const DatabaseCoverId& id) const;

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
	void initial();

	void showImportMenu(const QPoint& pos);

	void showPlaylistContextMenu(const QPoint& pos);

	void loadLocalFile();

	void loadFileDirectory();

	void clearAll();

	void playIndex(const QModelIndex& index, bool is_play);

	void showProgressPage();

	void requestMissingAlbumCovers();

    int32_t playlist_id_{ kDefaultPlaylistId };
    QString playlist_name_;
    PlaybackSnapshot playback_;
    bool queue_visible_{ false };
    QLabel* playlist_cover_{};
    QLabel* playlist_title_{};
    QLabel* playlist_stats_{};
    QPushButton* play_button_{};
    QPushButton* shuffle_button_{};
	RichPlaylistCoverPanel* cover_panel_{ nullptr };
	RichPlaylistView* rich_playlist_view_{ nullptr };
	ScanFileProgressPage* progress_page_{ nullptr };
	QSet<int32_t> requested_album_cover_ids_;
};
