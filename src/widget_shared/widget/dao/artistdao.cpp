#include <widget/dao/artistdao.h>

namespace dao {
    ArtistDao::ArtistDao()
        : ArtistDao(qGuiDb.getDatabase()) {
    }

	ArtistDao::ArtistDao(QSqlDatabase& db)
		: db_(db) {
	}

	void ArtistDao::updateArtistCoverId(int32_t artist_id, const QString& cover_id) {
		SqlQuery query(db_);

		query.prepare(qTEXT("UPDATE artists SET coverId = :coverId WHERE (artistId = :artistId)"));

		query.bindValue(qTEXT(":artistId"), artist_id);
		query.bindValue(qTEXT(":coverId"), cover_id);

		DbIfFailedThrow1(query);
	}

	std::optional<ArtistStats> ArtistDao::getArtistStats(int32_t artist_id) const {
        SqlQuery query(db_);

        query.prepare(qTEXT(R"(
    SELECT
        SUM(musics.duration) AS durations,
        (SELECT COUNT( * ) AS albums FROM albums WHERE albums.artistId = :artistId) AS albums,
        (SELECT COUNT( * ) AS tracks FROM albumMusic WHERE albumMusic.artistId = :artistId) AS tracks
    FROM
        albumMusic
    JOIN albums ON albums.artistId = albumMusic.artistId
    JOIN musics ON musics.musicId = albumMusic.musicId
    WHERE
        albums.artistId = :artistId;)"));

        query.bindValue(qTEXT(":artistId"), artist_id);

        DbIfFailedThrow1(query);

        while (query.next()) {
            ArtistStats stats;
            stats.albums = query.value(qTEXT("albums")).toInt();
            stats.tracks = query.value(qTEXT("tracks")).toInt();
            stats.durations = query.value(qTEXT("durations")).toDouble();
            return std::optional<ArtistStats>{ std::in_place_t{}, stats };
        }

        return std::nullopt;
	}

    int32_t ArtistDao::addOrUpdateArtist(const QString& artist) {
        XAMP_EXPECTS(!artist.isEmpty());

        SqlQuery query(db_);

        query.prepare(qTEXT(R"(
    INSERT OR REPLACE INTO artists (artistId, artist, firstChar)
    VALUES ((SELECT artistId FROM artists WHERE artist = :artist), :artist, :firstChar)
    )"));

        const auto first_char = artist.left(1);
        query.bindValue(qTEXT(":artist"), artist);
        query.bindValue(qTEXT(":firstChar"), first_char.toUpper());

        DbIfFailedThrow1(query);

        const auto artist_id = query.lastInsertId().toInt();
        return artist_id;
    }

    void ArtistDao::updateArtistEnglishName(const QString& artist, const QString& en_name) {
        SqlQuery query(db_);

        query.prepare(qTEXT("UPDATE artists SET artistNameEn = :artistNameEn, firstCharEn = :firstCharEn WHERE (artist = :artist AND firstCharEn IS NULL)"));

        query.bindValue(qTEXT(":artist"), artist);
        query.bindValue(qTEXT(":artistNameEn"), en_name);
        query.bindValue(qTEXT(":firstCharEn"), en_name.left(1));

        DbIfFailedThrow1(query);
    }

    QString ArtistDao::getArtistCoverId(int32_t artist_id) const {
        SqlQuery query(db_);

        query.prepare(qTEXT("SELECT coverId FROM artists WHERE artistId = (:artistId)"));
        query.bindValue(qTEXT(":artistId"), artist_id);

        DbIfFailedThrow1(query);

        const auto index = query.record().indexOf(qTEXT("coverId"));
        if (query.next()) {
            return query.value(index).toString();
        }
        return kEmptyString;
    }

    void ArtistDao::removeArtistId(int32_t artist_id) {
        SqlQuery query(db_);
        query.prepare(qTEXT("DELETE FROM artists WHERE artistId=:artistId"));
        query.bindValue(qTEXT(":artistId"), artist_id);
        DbIfFailedThrow1(query);
    }

    void ArtistDao::updateArtist(int32_t artist_id, const QString& artist)  {
    }

    void ArtistDao::removeAllArtist() {
        SqlQuery query(db_);
        query.prepare(qTEXT("DELETE FROM artists"));
        DbIfFailedThrow1(query);
    }

    void ArtistDao::updateArtistByDiscId(const QString& disc_id, const QString& artist) {
        XAMP_EXPECTS(!artist.isEmpty());

        SqlQuery query(db_);

        query.prepare(qTEXT(R"(
        SELECT
            albumMusic.artistId
        FROM
            albumMusic
        JOIN albums ON albums.albumId = albumMusic.albumId
        WHERE
            albums.discId = :discId
        LIMIT 1
        )"));

        query.bindValue(qTEXT(":discId"), disc_id);
        DbIfFailedThrow1(query);

        const auto index = query.record().indexOf(qTEXT("artistId"));
        if (query.next()) {
            auto artist_id = query.value(index).toInt();
            query.prepare(qTEXT("UPDATE artists SET artist = :artist WHERE (artistId = :artistId)"));
            query.bindValue(qTEXT(":artistId"), artist_id);
            query.bindValue(qTEXT(":artist"), artist);
            DbIfFailedThrow1(query);
        }
    }
}
