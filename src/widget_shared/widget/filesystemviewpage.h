//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <widget/widget_shared_global.h>
#include <widget/widget_shared.h>
#include <metadata/metadatalibraryscanner.h>
#include <QFrame>

#include <atomic>
#include <memory>
#include <stop_token>

class PlaylistPage;
class FileSystemModel;
class SpectrogramWidget;
class ScanFileProgressPage;

namespace Ui {
	class FileSystemViewPage;
}

class XAMP_WIDGET_SHARED_EXPORT FileSystemViewPage : public QFrame {
	Q_OBJECT
public:
	explicit FileSystemViewPage(QWidget* parent = nullptr);

	virtual ~FileSystemViewPage() override;

    void setScannerThreadPool(std::shared_ptr<IThreadPoolExecutor> scanner_thread_pool);

	PlaylistPage* playlistPage();

	SpectrogramWidget* spectrogramWidget();
signals:
	void addPathToPlaylist(const QString& path, bool append_to_playlist);

private:
	class DirFirstSortFilterProxyModel;
	class DisableToolTipStyledItemDelegate;

	QString file_path_;
	Ui::FileSystemViewPage* ui_;
	FileSystemModel* dir_model_;
	ScanFileProgressPage* progress_page_;
	DirFirstSortFilterProxyModel* dir_first_sort_filter_;
	std::shared_ptr<MetadataLibraryScanner> metadata_scanner_;
	std::shared_ptr<std::stop_source> scanner_stop_source_;
	std::atomic_uint64_t scanner_generation_{ 0 };
};
