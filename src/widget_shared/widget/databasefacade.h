//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>

#include <widget/widget_shared.h>
#include <widget/widget_shared_global.h>

class Database;

const std::wstring kUnknownAlbum{ L"Unknown album" };
const std::wstring kUnknownArtist{ L"Unknown artist" };

class XAMP_WIDGET_SHARED_EXPORT DatabaseFacade final : public QObject {
	Q_OBJECT
public:
    static constexpr size_t kReserveSize = 1024;
    
    explicit DatabaseFacade(QObject* parent = nullptr, Database *database = nullptr);

    void initUnknownAlbumAndArtist();

signals:
    void findAlbumCover(int32_t album_id, const std::wstring& file_path);

public:
    void insertTrackInfo(const ForwardList<TrackInfo>& result, int32_t playlist_id,
        std::function<void(int32_t)> fetch_cover = nullptr);

private:    
    void addTrackInfo(const ForwardList<TrackInfo>& result, int32_t playlist_id,    
        std::function<void(int32_t)> fetch_cover);

    bool is_stop_{false};

    int32_t unknown_artist_id_{ -1 };
    int32_t unknown_album_id_{ -1 };
    LoggerPtr logger_;
    Database* database_;
};

