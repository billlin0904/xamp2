//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>


#include <widget/widget_shared.h>
#include <widget/widget_shared_global.h>
#include <widget/database.h>

#include <base/lazy_storage.h>

const std::wstring kUnknownAlbum{ L"Unknown album" };
const std::wstring kUnknownArtist{ L"Unknown artist" };

struct UnknownArtistAndAlbumId : public std::pair<int32_t, int32_t> {
    UnknownArtistAndAlbumId();
};

class XAMP_WIDGET_SHARED_EXPORT DatabaseFacade final : public QObject {
	Q_OBJECT
public:
    static constexpr size_t kReserveSize = 1024;
    
    explicit DatabaseFacade(QObject* parent = nullptr, Database *database = nullptr);

    static int32_t unknownArtistId();

    static int32_t unknownAlbumId();

signals:
    void findAlbumCover(int32_t album_id, const std::wstring& file_path);

public:
    void insertTrackInfo(const ForwardList<TrackInfo>& result,
        int32_t playlist_id,
        StoreType store_type,
        std::function<void(int32_t)> fetch_cover = nullptr);

private:    
    void addTrackInfo(const ForwardList<TrackInfo>& result,
        int32_t playlist_id,
        StoreType store_type,
        std::function<void(int32_t)> fetch_cover);

    bool is_stop_{false};

    static LocalStorage<UnknownArtistAndAlbumId> unknown_id_;
    LoggerPtr logger_;
    Database* database_;
};

