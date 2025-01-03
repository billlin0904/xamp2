//=====================================================================================================================
// Copyright (c) 2018-2025 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <widget/widget_shared_global.h>
#include <QFrame>

#include <QThread>

class PlaylistPage;
class FileSystemModel;

namespace Ui {
	class FileSystemViewPage;
}

class XAMP_WIDGET_SHARED_EXPORT FileSystemViewPage : public QFrame {
	Q_OBJECT
public:
	explicit FileSystemViewPage(QWidget* parent = nullptr);

	virtual ~FileSystemViewPage() override;

signals:
	void addPathToPlaylist(const QString& path, bool append_to_playlist);

private:
	class DirFirstSortFilterProxyModel;
	class DisableToolTipStyledItemDelegate;

	Ui::FileSystemViewPage* ui_;
	FileSystemModel* dir_model_;
	DirFirstSortFilterProxyModel* dir_first_sort_filter_;
};
