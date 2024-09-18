//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>
#include <QTimer>

#include <widget/playlistentity.h>
#include <widget/widget_shared_global.h>
#include <widget/widget_shared.h>
#include <widget/database.h>
#include <widget/databasefacade.h>
#include <widget/filesystemwatcher.h>

class AudioEmbeddingService;

class XAMP_WIDGET_SHARED_EXPORT FileSystemService final : public QObject {
    Q_OBJECT
public:
    static constexpr size_t kReserveFilePathSize = 1024;

    FileSystemService();

    virtual ~FileSystemService() override;

signals:
    void insertDatabase(const ForwardList<TrackInfo>& result, int32_t playlist_id);

    void readFileStart();

    void readCompleted();

    void readFilePath(const QString& file_path);

    void readFileProgress(int32_t progress);

    void foundFileCount(size_t file_count);

    void remainingTimeEstimation(size_t total_work, size_t completed_work, int32_t secs);

public slots:
    void onExtractFile(const QString& file_path, int32_t playlist_id);

    void onSetWatchDirectory(const QString& dir);

    void cancelRequested();

private:
    void scanPathFiles(int32_t playlist_id, const QString& dir);

    void updateProgress();

    bool is_stop_{ false };
    size_t total_work_{ 0 };
    std::atomic<size_t> completed_work_{ 0 };
    FileSystemWatcher watcher_;
    Stopwatch total_time_elapsed_;
    Stopwatch update_ui_elapsed_;
    QTimer timer_;
	AlignPtr<IThreadPoolExecutor> thread_pool_;
    LoggerPtr logger_;
};

