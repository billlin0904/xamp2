//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFrame>
#include <QPoint>

#include <base/trackinfo.h>

#include <widget/tabpage.h>
#include <widget/widget_shared_global.h>

class PlaylistTableView;
class QPixmap;
class QScrollArea;
class QVBoxLayout;
class RichPlaylistAlbumHeaderPanel;
class RichPlaylistCoverPanel;

class XAMP_WIDGET_SHARED_EXPORT RichPlaylistPage final : public QFrame, public TabPage {
public:
	explicit RichPlaylistPage(QWidget* parent = nullptr);

	void reload() override;

	PlaylistTableView* playlist() const {
		return playlist_;
	}

	void setNowPlaying(const xamp::base::TrackInfo& track_info, const QPixmap& cover);

	void clearNowPlaying();

private:
	void initial();

	void updateAlbumHeaderFromPlaylist();

	void rebuildAlbumGroups();

	void showImportMenu(const QPoint& pos);

	void loadLocalFile();

	void loadFileDirectory();

	RichPlaylistCoverPanel* cover_panel_{ nullptr };
	QScrollArea* album_groups_scroll_area_{ nullptr };
	QWidget* album_groups_widget_{ nullptr };
	QVBoxLayout* album_groups_layout_{ nullptr };
	PlaylistTableView* playlist_{ nullptr };
};
