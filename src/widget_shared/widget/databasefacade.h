//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>

#include <widget/widget_shared.h>
#include <widget/widget_shared_global.h>

class Database;

class XAMP_WIDGET_SHARED_EXPORT DatabaseFacade final : public QObject {
	Q_OBJECT
public:
    static constexpr size_t kReserveSize = 1024;
    
    explicit DatabaseFacade(QObject* parent = nullptr, Database *database = nullptr);

signals:
    void findAlbumCover(int32_t album_id, const std::wstring& file_path);
public:
    void insertTrackInfo(const ForwardList<TrackInfo>& result, int32_t playlist_id,
        std::function<void(int32_t)> fetch_cover = nullptr);

private:   
    void addTrackInfo(const ForwardList<TrackInfo>& result, int32_t playlist_id,    
        std::function<void(int32_t)> fetch_cover);

    bool is_stop_{false};
    LoggerPtr logger_;
    Database* database_;
};

