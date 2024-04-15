//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>

#include <widget/widget_shared.h>
#include <widget/widget_shared_global.h>
#include <widget/database.h>

class XAMP_WIDGET_SHARED_EXPORT DatabaseFacade final : public QObject {
	Q_OBJECT
public:
    static constexpr size_t kReserveSize = 1024;
    
    explicit DatabaseFacade(QObject* parent = nullptr, Database *database = nullptr);

    QString unknownArtist() const {
        return unknown_artist_;
    }

    QString unknownAlbum() const {
        return unknown_album_;
    }

    int32_t unknownArtistId() const;

    int32_t unknownAlbumId() const;

    void insertTrackInfo(const ForwardList<TrackInfo>& result,
        int32_t playlist_id,
        StoreType store_type,
        const std::function<void(int32_t, int32_t)>& fetch_cover = nullptr);

private:    
    void ensureInitialUnknownId();

    void addTrackInfo(const ForwardList<TrackInfo>& result,
        int32_t playlist_id,
        StoreType store_type,
        const std::function<void(int32_t, int32_t)>& fetch_cover);

    bool is_stop_{false};
    int32_t unknown_artist_id_{ kInvalidDatabaseId };
    int32_t unknown_album_id_{ kInvalidDatabaseId };
    QString unknown_artist_;
    QString unknown_album_;
    LoggerPtr logger_;
    Database* database_;
};

#define qDatabaseFacade SharedSingleton<DatabaseFacade>::GetInstance()