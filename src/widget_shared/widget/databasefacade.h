//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>

#include <widget/widget_shared.h>
#include <widget/widget_shared_global.h>

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
    void FindAlbumCover(int32_t album_id,
        const QString& album,
        const std::wstring& file_path);
public:
    void InsertTrackInfo(const ForwardList<TrackInfo>& result, int32_t playlist_id);

private:   
    void AddTrackInfo(const ForwardList<TrackInfo>& result, int32_t playlist_id);

    bool is_stop_{false};
    LoggerPtr logger_;
};

