//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>

#include <widget/playlistentity.h>
#include <widget/widget_shared_global.h>
#include <widget/widget_shared.h>
#include <widget/database.h>

class XAMP_WIDGET_SHARED_EXPORT ExtractFileWorker : public QObject {
    Q_OBJECT
public:
    static constexpr size_t kReserveSize = 1024;

    ExtractFileWorker();

    void cancelRequested();

signals:
    void insertDatabase(const ForwardList<TrackInfo>& result, int32_t playlist_id);

    void readFileStart();

    void readCompleted();

    void readFilePath(const QString& file_path);

    void readFileProgress(int32_t progress);

    void foundFileCount(size_t file_count);

public slots:
    void onExtractFile(const QString& file_path, int32_t playlist_id);

private:
    size_t scanPathFiles(const QStringList& file_name_filters,
        int32_t playlist_id,
        const QString& dir);

    bool is_stop_{ true };
    LoggerPtr logger_;
};

