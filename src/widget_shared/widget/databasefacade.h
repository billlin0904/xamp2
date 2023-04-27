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
    void ReadFileStart(int dir_size);

    void ReadFileProgress(int progress);

    void ReadFileEnd();

    void FromDatabase(const ForwardList<PlayListEntity>& entity);

	void ReadCompleted(int32_t total_album, int32_t total_tracks);

    void ReadCurrentFilePath(const QString& dir, int32_t total_tracks, int32_t num_track);

public:
    void ReadTrackInfo(QString const& file_path,
        int32_t playlist_id,
        bool is_podcast_mode);

    void InsertTrackInfo(const ForwardList<TrackInfo>& result, 
        int32_t playlist_id, 
        bool is_podcast_mode);

    static void FindAlbumCover(int32_t album_id,
        const QString& album,
        const std::wstring& 
        file_path, 
        const CoverArtReader &reader);

private:
    void ScanPathFiles(const QStringList& file_name_filters,
        const QString& dir,
        int32_t playlist_id,
        bool is_podcast_mode);

    void AddTrackInfo(const ForwardList<TrackInfo>& result,
        int32_t playlist_id,
        bool is_podcast);

    QSet<QString> GetAlbumCategories(const QString& album) const;

    bool is_stop_{false};
    LoggerPtr logger_;
    QEventLoop event_loop_;
    QTimer timer_;
};

