//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFrame>
#include <QPoint>
#include <QSet>

#include <base/trackinfo.h>

#include <widget/databasecoverid.h>
#include <widget/playlistentity.h>
#include <widget/tabpage.h>
#include <widget/widget_shared.h>
#include <widget/widget_shared_global.h>

class QPixmap;
class QModelIndex;
class RichPlaylistCoverPanel;
class RichPlaylistView;
class ScanFileProgressPage;

class XAMP_WIDGET_SHARED_API RichPlaylistPage final : public QFrame, public TabPage {
	Q_OBJECT
public:
	explicit RichPlaylistPage(QWidget* parent = nullptr);

	void reload() override;

	void setNowPlaying(const TrackInfo& track_info, const QPixmap& cover);

	void clearNowPlaying();

	void onAlbumCoverLoaded(int32_t album_id);

	ScanFileProgressPage* progressPage() const;

	bool playNextItem(int32_t forward);

	void loadPath(const QString& file_path, bool append_to_playlist);

signals:
	void playMusic(int32_t playlist_id, const PlayListEntity& item, bool is_play);

	void extractFile(const QString& file_path, int32_t playlist_id);

	void findAlbumCover(const DatabaseCoverId& id) const;

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

	RichPlaylistCoverPanel* cover_panel_{ nullptr };
	RichPlaylistView* rich_playlist_view_{ nullptr };
	ScanFileProgressPage* progress_page_{ nullptr };
	QSet<int32_t> requested_album_cover_ids_;
};
