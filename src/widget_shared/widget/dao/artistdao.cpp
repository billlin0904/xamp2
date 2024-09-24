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

		query.prepare("UPDATE artists SET coverId = :coverId WHERE (artistId = :artistId)"_str);

		query.bindValue(":artistId"_str, artist_id);
		query.bindValue(":coverId"_str, cover_id);

		DbIfFailedThrow1(query);
	}

	std::optional<ArtistStats> ArtistDao::getArtistStats(int32_t artist_id) const {
        SqlQuery query(db_);

        query.prepare(R"(
    SELECT
        SUM(musics.duration) AS durations,
        (SELECT COUNT( * ) AS albums FROM albums WHERE albums.artistId = :artistId) AS albums,
        (SELECT COUNT( * ) AS tracks FROM albumMusic WHERE albumMusic.artistId = :artistId) AS tracks
    FROM
        albumMusic
    JOIN albums ON albums.artistId = albumMusic.artistId
    JOIN musics ON musics.musicId = albumMusic.musicId
    WHERE
        albums.artistId = :artistId;)"_str);

        query.bindValue(":artistId"_str, artist_id);

        DbIfFailedThrow1(query);

        while (query.next()) {
            ArtistStats stats;
            stats.albums = query.value("albums"_str).toInt();
            stats.tracks = query.value("tracks"_str).toInt();
            stats.durations = query.value("durations"_str).toDouble();
            return std::optional<ArtistStats>{ std::in_place_t{}, stats };
        }

        return std::nullopt;
	}

    int32_t ArtistDao::addOrUpdateArtist(const QString& artist, const QString& first_char) {
        XAMP_EXPECTS(!artist.isEmpty());

        SqlQuery query(db_);

        query.prepare(R"(
    INSERT OR REPLACE INTO artists (artistId, artist, firstChar)
    VALUES ((SELECT artistId FROM artists WHERE artist = :artist), :artist, :firstChar)
    )"_str);

        query.bindValue(":artist"_str, artist);
        if (first_char.isEmpty()) {
            const auto fc = artist.left(1);
            query.bindValue(":firstChar"_str, fc.toUpper());
        } else {
            query.bindValue(":firstChar"_str, first_char.toUpper());
        }

        DbIfFailedThrow1(query);

        const auto artist_id = query.lastInsertId().toInt();
        return artist_id;
    }

    void ArtistDao::updateArtistEnglishName(const QString& artist, const QString& en_name) {
        SqlQuery query(db_);

        query.prepare("UPDATE artists SET artistNameEn = :artistNameEn, firstCharEn = :firstCharEn WHERE (artist = :artist AND firstCharEn IS NULL)"_str);

        query.bindValue(":artist"_str, artist);
        query.bindValue(":artistNameEn"_str, en_name);
        query.bindValue(":firstCharEn"_str, en_name.left(1));

        DbIfFailedThrow1(query);
    }

	int32_t ArtistDao::getArtistId(const QString& artist) const {
		SqlQuery query(db_);
		query.prepare("SELECT artistId FROM artists WHERE artist = :artist"_str);
		query.bindValue(":artist"_str, artist);
		DbIfFailedThrow1(query);
        const auto index = query.record().indexOf("artistId"_str);
        if (query.next()) {
            return query.value(index).toInt();
        }
        return kInvalidDatabaseId;
	}

    QString ArtistDao::getArtistCoverId(int32_t artist_id) const {
        SqlQuery query(db_);

        query.prepare("SELECT coverId FROM artists WHERE artistId = (:artistId)"_str);
        query.bindValue(":artistId"_str, artist_id);

        DbIfFailedThrow1(query);

        const auto index = query.record().indexOf("coverId"_str);
        if (query.next()) {
            return query.value(index).toString();
        }
        return kEmptyString;
    }

    void ArtistDao::removeArtistId(int32_t artist_id) {
        SqlQuery query(db_);
        query.prepare("DELETE FROM artists WHERE artistId=:artistId"_str);
        query.bindValue(":artistId"_str, artist_id);
        DbIfFailedThrow1(query);
    }

    void ArtistDao::updateArtist(int32_t artist_id, const QString& artist)  {
    }

    void ArtistDao::removeAllArtist() {
        SqlQuery query(db_);
        query.prepare("DELETE FROM artists"_str);
        DbIfFailedThrow1(query);
    }

    void ArtistDao::updateArtistByDiscId(const QString& disc_id, const QString& artist) {
        XAMP_EXPECTS(!artist.isEmpty());

        SqlQuery query(db_);

        query.prepare(R"(
        SELECT
            albumMusic.artistId
        FROM
            albumMusic
        JOIN albums ON albums.albumId = albumMusic.albumId
        WHERE
            albums.discId = :discId
        LIMIT 1
        )"_str);

        query.bindValue(":discId"_str, disc_id);
        DbIfFailedThrow1(query);

        const auto index = query.record().indexOf("artistId"_str);
        if (query.next()) {
            auto artist_id = query.value(index).toInt();
            query.prepare("UPDATE artists SET artist = :artist WHERE (artistId = :artistId)"_str);
            query.bindValue(":artistId"_str, artist_id);
            query.bindValue(":artist"_str, artist);
            DbIfFailedThrow1(query);
        }
    }
}
