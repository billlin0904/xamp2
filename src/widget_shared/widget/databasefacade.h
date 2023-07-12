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

class ExtractFileWorker;
class PlayListTableView;

TrackInfo GetTrackInfo(QString const& file_path);

const QStringList& GetFileNameFilter();

class CoverArtReader final {
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
    static constexpr size_t kReserveSize = 1024;
    
    explicit DatabaseFacade(QObject* parent = nullptr);

signals:    
    void InsertDatabase(const ForwardList<TrackInfo>& result,
        int32_t playlist_id,
        bool is_podcast_mode);
    
    void FindAlbumCover(int32_t album_id,
        const QString& album,
        const std::wstring& file_path);
public:
    static QStringList NormalizeGenre(const QString& genre);

    void InsertTrackInfo(const Vector<TrackInfo>& result,
        int32_t playlist_id, 
        bool is_podcast_mode);

private:   
    void AddTrackInfo(const Vector<TrackInfo>& result,
        int32_t playlist_id,
        bool is_podcast);

    bool is_stop_{false};
    LoggerPtr logger_;
};

