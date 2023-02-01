//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>
#include <QMutex>

#include <widget/widget_shared.h>
#include <widget/playlistentity.h>

class PlayListTableView;

TrackInfo GetTrackInfo(QString const& file_path);

QString GetFileDialogFileExtensions();

QStringList GetFileNameFilter();

class CoverArtReader final {
public:
    CoverArtReader();

    QPixmap GetEmbeddedCover(const TrackInfo& track_info) const;

    QPixmap GetEmbeddedCover(const Path& file_path) const;

private:
    AlignPtr<IMetadataReader> cover_reader_;
};

class DatabaseFacade final : public QObject {
	Q_OBJECT
public:
    explicit DatabaseFacade(QObject* parent = nullptr);

signals:
    void ReadFileStart(int dir_size);

    void ReadFileProgress(const QString &dir, int progress);

    void ReadFileEnd();

    void FromDatabase(const ForwardList<PlayListEntity>& entity);

	void ReadCompleted(const ForwardList<TrackInfo> &entity);

public:
    static void InsertTrackInfo(const ForwardList<TrackInfo>& result, 
        int32_t playlist_id, 
        bool is_podcast_mode);

    static void FindAlbumCover(int32_t album_id, const std::wstring& album, const std::wstring& file_path, const CoverArtReader &reader);

private:
    static void AddTrackInfo(const ForwardList<TrackInfo>& result,
        int32_t playlist_id,
        int64_t dir_last_write_time,
        bool is_podcast);
};

