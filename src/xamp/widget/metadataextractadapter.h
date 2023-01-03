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

    QPixmap getEmbeddedCover(const TrackInfo& metadata) const;

    std::tuple<int32_t, int32_t, QString> addOrGetAlbumAndArtistId(int64_t dir_last_write_time,
        const QString& album,
        const QString& artist,
        bool is_podcast,
        const QString& disc_id) const;

    QString addCoverCache(int32_t album_id, const QString& album, const TrackInfo& metadata, bool is_unknown_album) const;

    void clear();

private:
    mutable LruCache<int32_t, QString> cover_id_cache_;
    // Key: Album + Artist
    mutable LruCache<QString, int32_t> album_id_cache_;
    mutable LruCache<QString, int32_t> artist_id_cache_;
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

    static void readFileMetadata(const QSharedPointer<MetadataExtractAdapter>& adapter, QString const& file_path, bool show_progress_dialog = true);

signals:
    void fromDatabase(const ForwardList<PlayListEntity>& entity);

	void readCompleted(int64_t dir_last_write_time, const ForwardList<TrackInfo> &entity);

public:
    static void ScanDirFiles(const QSharedPointer<MetadataExtractAdapter>& adapter, const QString& dir);

    static void processMetadata(const ForwardList<TrackInfo>& result, PlayListTableView *playlist = nullptr, int64_t dir_last_write_time = -1);

	static void addMetadata(const ForwardList<TrackInfo>& result, PlayListTableView* playlist, int64_t dir_last_write_time, bool is_podcast);
};

