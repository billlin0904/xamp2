//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>
#include <QTimer>

#include <stop_token>

#include <widget/playlistentity.h>
#include <widget/widget_shared_global.h>
#include <widget/widget_shared.h>
#include <widget/database.h>
#include <widget/databasefacade.h>

class XAMP_WIDGET_SHARED_EXPORT FileSystemService final : public QObject {
    Q_OBJECT
public:
    static constexpr size_t kReserveFilePathSize = 1024;

    FileSystemService();

    virtual ~FileSystemService() override;

signals:
    void insertDatabase(const std::forward_list<TrackInfo>& result, int32_t playlist_id);

    void batchInsertDatabase(const std::vector<std::forward_list<TrackInfo>>& results, int32_t playlist_id);

    void readFileStart();

    void readCompleted();

    void readFilePath(const QString& file_path);

    void readFileProgress(int32_t progress);

    void foundFileCount(size_t file_count);

    void remainingTimeEstimation(size_t total_work, size_t completed_work, int32_t secs);

    void enableFetchThumbnail(bool enable);

    void scanReplayGainCompleted(const QList<PlayListEntity>& entities);

    void scanReplayGainError();

public slots:
    void scanReplayGain(const QList<PlayListEntity>& entities);

    void onExtractFile(const QString& file_path, int32_t playlist_id);

    void cancelRequested();

private:
    void scanPathFiles(int32_t playlist_id, const QString& dir);

    void updateProgress();

    bool is_stop_{ false };
    int32_t playlist_id_{ 0 };
    size_t total_work_{ 0 };
    size_t last_completed_work_{ 0 };
    std::atomic<size_t> completed_work_{ 0 };
    Stopwatch total_time_elapsed_;
    Stopwatch update_ui_elapsed_;
    FastMutex batch_mutex_;
    QTimer timer_;
    std::shared_ptr<IThreadPoolExecutor> thread_pool_;
    std::stop_source stop_source_;
    QScopedPointer<Database> database_;
    LoggerPtr logger_;
};

