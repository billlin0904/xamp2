//=====================================================================================================================
// Copyright (c) 2018-2025 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QString>
#include <widget/widget_shared.h>
#include <widget/database.h>

namespace dao {

class XAMP_WIDGET_SHARED_EXPORT ArtistDao final {
public:
    ArtistDao();

    explicit ArtistDao(QSqlDatabase& db);

    void updateArtistCoverId(int32_t artist_id, const QString& cover_id);
    std::optional<ArtistStats> getArtistStats(int32_t artist_id) const;
    int32_t addOrUpdateArtist(const QString& artist, const QString &first_char = QString());
    void updateArtistEnglishName(const QString& artist, const QString& en_name);
    QString getArtistCoverId(int32_t artist_id) const;
    void removeArtistId(int32_t artist_id);
    void updateArtist(int32_t artist_id, const QString& artist);
    void removeAllArtist();
    void updateArtistByDiscId(const QString& disc_id, const QString& artist);
    int32_t getArtistId(const QString& artist) const;
private:
    QSqlDatabase& db_;
};

}