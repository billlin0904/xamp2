//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>

#include <widget/playlistentity.h>
#include <widget/widget_shared.h>

class PlayListTableView;

TrackInfo getMetadata(QString const& file_path);

QString getFileDialogFileExtensions();

QStringList getFileNameFilter();

class DatabaseIdCache final {
public:
    DatabaseIdCache();

    QPixmap getEmbeddedCover(const TrackInfo& track_info) const;

    static std::tuple<int32_t, int32_t> addOrGetAlbumAndArtistId(int64_t dir_last_write_time,
                                                                 const QString& album,
                                                                 const QString& artist,
                                                                 bool is_podcast,
                                                                 const QString& disc_id);

    QString addCoverCache(int32_t album_id, const QString& album, const TrackInfo& track_info, bool is_unknown_album) const;

private:
    AlignPtr<IMetadataReader> cover_reader_;
};

#define qDatabaseIdCache SharedSingleton<DatabaseIdCache>::GetInstance()

class MetadataExtractAdapter final
	: public QObject {
	Q_OBJECT
public:
    static constexpr uint64_t kDirHashKey1 = 0x7720796f726c694bUL;
    static constexpr uint64_t kDirHashKey2 = 0x2165726568207361UL;

    explicit MetadataExtractAdapter(QObject* parent = nullptr);

signals:
    void readFileStart(int dir_size);

    void readFileProgress(const QString &dir, int progress);

    void readFileEnd();

    void fromDatabase(const ForwardList<PlayListEntity>& entity);

	void readCompleted(int64_t dir_last_write_time, const ForwardList<TrackInfo> &entity);

public:
    static void processMetadata(const ForwardList<TrackInfo>& result, 
        int64_t dir_last_write_time, 
        int32_t playlist_id, 
        bool is_podcast_mode);

	static void addMetadata(const ForwardList<TrackInfo>& result,
        int32_t playlist_id,
        int64_t dir_last_write_time,
        bool is_podcast);
};

