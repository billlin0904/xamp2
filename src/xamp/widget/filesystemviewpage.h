//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <ui_filesystemviewpage.h>

class PlaylistPage;
class FileSystemModel;

class FileSystemViewPage : public QFrame {
	Q_OBJECT
public:
	explicit FileSystemViewPage(QWidget* parent = nullptr);

	PlaylistPage* playlistPage();

signals:
	void addDirToPlaylist(const QString& url);

private:
	class DirFirstSortFilterProxyModel;

	Ui::FileSystemViewPage ui;
	FileSystemModel* dir_model_;
	DirFirstSortFilterProxyModel* dir_first_sort_filter_;
};