//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFrame>
#include <QPoint>

#include <base/trackinfo.h>

#include <widget/playlistentity.h>
#include <widget/tabpage.h>
#include <widget/widget_shared.h>
#include <widget/widget_shared_global.h>

class QPixmap;
class QModelIndex;
class RichPlaylistCoverPanel;
class RichPlaylistView;
class ScanFileProgressPage;

class XAMP_WIDGET_SHARED_EXPORT RichPlaylistPage final : public QFrame, public TabPage {
	Q_OBJECT
public:
	explicit RichPlaylistPage(QWidget* parent = nullptr);

	void reload() override;

	void setNowPlaying(const xamp::base::TrackInfo& track_info, const QPixmap& cover);

	void clearNowPlaying();

	ScanFileProgressPage* progressPage() const;

	bool playNextItem(int32_t forward);

signals:
	void playMusic(int32_t playlist_id, const PlayListEntity& item, bool is_play);

	void extractFile(const QString& file_path, int32_t playlist_id);

private:
	void initial();

	void showImportMenu(const QPoint& pos);

	void loadLocalFile();

	void loadFileDirectory();

	void clearAll();

	void playIndex(const QModelIndex& index, bool is_play);

	void showProgressPage();

	RichPlaylistCoverPanel* cover_panel_{ nullptr };
	RichPlaylistView* rich_playlist_view_{ nullptr };
	ScanFileProgressPage* progress_page_{ nullptr };
};
