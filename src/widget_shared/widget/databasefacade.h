//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>
#include <QEventLoop>
#include <QTimer>
#include <QRegularExpression>

#include <widget/widget_shared.h>
#include <widget/playlistentity.h>
#include <widget/widget_shared_global.h>

class BackgroundWorker;
class PlayListTableView;

TrackInfo GetTrackInfo(QString const& file_path);

QString GetFileDialogFileExtensions();

QStringList GetFileNameFilter();

class XAMP_WIDGET_SHARED_EXPORT CoverArtReader final {
public:
    CoverArtReader();

    QPixmap GetEmbeddedCover(const TrackInfo& track_info) const;

    QPixmap GetEmbeddedCover(const Path& file_path) const;

private:
    AlignPtr<IMetadataReader> cover_reader_;
};

class XAMP_WIDGET_SHARED_EXPORT DatabaseFacade final : public QObject {
	Q_OBJECT
public:
    explicit DatabaseFacade(QObject* parent = nullptr);

signals:    
    void InsertDatabase(const ForwardList<TrackInfo>& result,
        int32_t playlist_id,
        bool is_podcast_mode);
    
public:
    void ReadTrackInfo(BackgroundWorker *worker,
        QString const& file_path,
        int32_t playlist_id,
        bool is_podcast_mode);

    static void InsertTrackInfo(const ForwardList<TrackInfo>& result,
        int32_t playlist_id, 
        bool is_podcast_mode);

    static void FindAlbumCover(int32_t album_id,
        const QString& album,
        const std::wstring& 
        file_path, 
        const CoverArtReader &reader);

private:   
    static void AddTrackInfo(const ForwardList<TrackInfo>& result,
        int32_t playlist_id,
        bool is_podcast);

    static QSet<QString> GetAlbumCategories(const QString& album);

    void ScanPathFiles(BackgroundWorker* worker,
        HashMap<std::wstring, ForwardList<TrackInfo>>& album_groups,
        const QStringList& file_name_filters,
        const QString& dir,
        int32_t playlist_id,
        bool is_podcast_mode);

    bool is_stop_{false};
    LoggerPtr logger_;
};

