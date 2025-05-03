//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <widget/widget_shared_global.h>
#include <widget/widget_shared.h>
#include <QFrame>

class PlaylistPage;
class FileSystemModel;
class WaveformWidget;

namespace Ui {
	class FileSystemViewPage;
}

class XAMP_WIDGET_SHARED_EXPORT FileSystemViewPage : public QFrame {
	Q_OBJECT
public:
	explicit FileSystemViewPage(QWidget* parent = nullptr);

	virtual ~FileSystemViewPage() override;

	PlaylistPage* playlistPage();

	WaveformWidget* waveformWidget();
signals:
	void addPathToPlaylist(const QString& path, bool append_to_playlist);

private:
	class DirFirstSortFilterProxyModel;
	class DisableToolTipStyledItemDelegate;

	QString file_path_;
	Ui::FileSystemViewPage* ui_;
	FileSystemModel* dir_model_;
	DirFirstSortFilterProxyModel* dir_first_sort_filter_;
};
