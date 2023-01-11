//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>

#include <widget/widget_shared.h>
#include <widget/playlistentity.h>

class PlayListTableView;

TrackInfo getTrackInfo(QString const& file_path);

QString getFileDialogFileExtensions();

QStringList getFileNameFilter();

class CoverArtReader final {
public:
    CoverArtReader();

    QPixmap getEmbeddedCover(const TrackInfo& track_info) const;

    QString saveCoverCache(int32_t album_id, const QString& album, const TrackInfo& track_info, bool is_unknown_album) const;

private:
    AlignPtr<IMetadataReader> cover_reader_;
};

class DatabaseProxy final : public QObject {
	Q_OBJECT
public:
    static constexpr uint64_t kDirHashKey1 = 0x7720796f726c694bUL;
    static constexpr uint64_t kDirHashKey2 = 0x2165726568207361UL;

    explicit DatabaseProxy(QObject* parent = nullptr);

signals:
    void readFileStart(int dir_size);

    void readFileProgress(const QString &dir, int progress);

    void readFileEnd();

    void fromDatabase(const ForwardList<PlayListEntity>& entity);

	void readCompleted(int64_t dir_last_write_time, const ForwardList<TrackInfo> &entity);

public:
    static void processTrackInfo(const ForwardList<TrackInfo>& result, 
        int64_t dir_last_write_time, 
        int32_t playlist_id, 
        bool is_podcast_mode);

	static void addTrackInfo(const ForwardList<TrackInfo>& result,
        int32_t playlist_id,
        int64_t dir_last_write_time,
        bool is_podcast);
};

