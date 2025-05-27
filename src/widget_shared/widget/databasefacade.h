//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>

#include <widget/widget_shared.h>
#include <widget/widget_shared_global.h>
#include <widget/database.h>
#include <widget/dao/dbfacade.h>

class XAMP_WIDGET_SHARED_EXPORT DatabaseFacade final : public QObject {
public:
    static constexpr size_t kReserveSize = 1024;
    
    explicit DatabaseFacade(QObject* parent = nullptr, Database *database = nullptr);

    ~DatabaseFacade() override;

    void initialUnknownTranslateString();

    QString unknown() const {
		return unknown_;
	}

    QString unknownArtist() const {
        return unknown_artist_;
    }

    QString unknownAlbum() const {
        return unknown_album_;
    }

    int32_t unknownArtistId() const;

    int32_t unknownAlbumId() const;

	void insertMultipleTrackInfo(
        const std::vector<std::forward_list<TrackInfo>>& results,
	    int32_t playlist_id,
	    StoreType store_type = StoreType::LOCAL_STORE,
        const QString& dick_id = QString(),
        const std::function<void(int32_t, int32_t)>& fetch_cover = nullptr);

    void insertTrackInfo(const std::forward_list<TrackInfo>& result,
        int32_t playlist_id,
        StoreType store_type = StoreType::LOCAL_STORE,
        const QString &dick_id = QString(),
        const std::function<void(int32_t, int32_t)>& fetch_cover = nullptr);
private:    
    void ensureAddUnknownId();

    int32_t kUnknownArtistId{ kInvalidDatabaseId };
    int32_t kUnknownAlbumId{ kInvalidDatabaseId };
    int32_t kVariousArtistsId{ kInvalidDatabaseId };

    QString various_artists_;
    QString unknown_artist_;
    QString unknown_album_;
    QString unknown_;
    LoggerPtr logger_;
    Database* database_;
    QScopedPointer<dao::DatabaseFacade> dao_facade_;
};

#define qDatabaseFacade SharedSingleton<DatabaseFacade>::GetInstance()