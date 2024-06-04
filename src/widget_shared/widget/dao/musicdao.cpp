#include <widget/dao/albumdao.h>
#include <widget/dao/playlistdao.h>
#include <widget/dao/musicdao.h>

namespace dao {
    MusicDao::MusicDao()
        : MusicDao(qGuiDb.getDatabase()) {
    }

	MusicDao::MusicDao(QSqlDatabase& db)
		: db_(db) {
	}

    int32_t MusicDao::addOrUpdateMusic(const TrackInfo& track_info) {
        SqlQuery query(db_);

        query.prepare(qTEXT(R"(
    INSERT OR REPLACE INTO musics
    (musicId, title, track, path, fileExt, fileName, duration, durationStr, parentPath, bitRate, sampleRate, offset, dateTime, albumReplayGain, trackReplayGain, albumPeak, trackPeak, genre, comment, fileSize, heart)
    VALUES ((SELECT musicId FROM musics WHERE path = :path AND offset = :offset), :title, :track, :path, :fileExt, :fileName, :duration, :durationStr, :parentPath, :bitRate, :sampleRate, :offset, :dateTime, :albumReplayGain, :trackReplayGain, :albumPeak, :trackPeak, :genre, :comment, :fileSize, :heart)
    )")
        );

        query.bindValue(qTEXT(":title"), toQString(track_info.title));
        query.bindValue(qTEXT(":track"), track_info.track);
        query.bindValue(qTEXT(":path"), toQString(track_info.file_path));
        query.bindValue(qTEXT(":fileExt"), toQString(track_info.file_ext()));
        query.bindValue(qTEXT(":fileName"), toQString(track_info.file_name()));
        query.bindValue(qTEXT(":parentPath"), toQString(track_info.parent_path()));
        query.bindValue(qTEXT(":duration"), track_info.duration);
        query.bindValue(qTEXT(":durationStr"), formatDuration(track_info.duration));
        query.bindValue(qTEXT(":bitRate"), track_info.bit_rate);
        query.bindValue(qTEXT(":sampleRate"), track_info.sample_rate);
        query.bindValue(qTEXT(":offset"), track_info.offset ? track_info.offset.value() : 0);
        query.bindValue(qTEXT(":fileSize"), track_info.file_size);
        query.bindValue(qTEXT(":heart"), track_info.rating ? 1 : 0);

        if (track_info.replay_gain) {
            query.bindValue(qTEXT(":albumReplayGain"), track_info.replay_gain.value().album_gain);
            query.bindValue(qTEXT(":trackReplayGain"), track_info.replay_gain.value().track_gain);
            query.bindValue(qTEXT(":albumPeak"), track_info.replay_gain.value().album_peak);
            query.bindValue(qTEXT(":trackPeak"), track_info.replay_gain.value().track_peak);
        }
        else {
            query.bindValue(qTEXT(":albumReplayGain"), QVariant());
            query.bindValue(qTEXT(":trackReplayGain"), QVariant());
            query.bindValue(qTEXT(":albumPeak"), QVariant());
            query.bindValue(qTEXT(":trackPeak"), QVariant());
        }

        query.bindValue(qTEXT(":dateTime"), track_info.last_write_time);
        query.bindValue(qTEXT(":genre"), toQString(track_info.genre));
        query.bindValue(qTEXT(":comment"), toQString(track_info.comment));

        DbIfFailedThrow1(query);

        const auto music_id = query.lastInsertId().toInt();
        XAMP_ENSURES(music_id != kInvalidDatabaseId);
        return music_id;
    }

    void MusicDao::updateMusic(int32_t music_id, const TrackInfo& track_info) {
        SqlQuery query(db_);

        query.prepare(qTEXT(R"(
    UPDATE musics SET
    title = :title,
	track = :track,
	path = :path,
	fileExt = :fileExt,
	fileName= :fileName,
	duration= :duration,
	durationStr= :durationStr,
	parentPath= :parentPath,
	bitRate= :bitRate,
	sampleRate= :sampleRate,
	offset= :offset,
	dateTime= :dateTime,
	albumReplayGain= :albumReplayGain,
	trackReplayGain= :trackReplayGain, 
	albumPeak= :albumPeak,
	trackPeak= :trackPeak,
	genre= :genre, 
	comment= :comment,
	fileSize= :fileSize,
	heart= :heart    
	WHERE musicId = :musicId
    )")
        );

        query.bindValue(qTEXT(":title"), toQString(track_info.title));
        query.bindValue(qTEXT(":track"), track_info.track);
        query.bindValue(qTEXT(":path"), toQString(track_info.file_path));
        query.bindValue(qTEXT(":fileExt"), toQString(track_info.file_ext()));
        query.bindValue(qTEXT(":fileName"), toQString(track_info.file_name()));
        query.bindValue(qTEXT(":parentPath"), toQString(track_info.parent_path()));
        query.bindValue(qTEXT(":duration"), track_info.duration);
        query.bindValue(qTEXT(":durationStr"), formatDuration(track_info.duration));
        query.bindValue(qTEXT(":bitRate"), track_info.bit_rate);
        query.bindValue(qTEXT(":sampleRate"), track_info.sample_rate);
        query.bindValue(qTEXT(":offset"), track_info.offset ? track_info.offset.value() : 0);
        query.bindValue(qTEXT(":fileSize"), track_info.file_size);
        query.bindValue(qTEXT(":heart"), track_info.rating ? 1 : 0);

        if (track_info.replay_gain) {
            query.bindValue(qTEXT(":albumReplayGain"), track_info.replay_gain.value().album_gain);
            query.bindValue(qTEXT(":trackReplayGain"), track_info.replay_gain.value().track_gain);
            query.bindValue(qTEXT(":albumPeak"), track_info.replay_gain.value().album_peak);
            query.bindValue(qTEXT(":trackPeak"), track_info.replay_gain.value().track_peak);
        }
        else {
            query.bindValue(qTEXT(":albumReplayGain"), QVariant());
            query.bindValue(qTEXT(":trackReplayGain"), QVariant());
            query.bindValue(qTEXT(":albumPeak"), QVariant());
            query.bindValue(qTEXT(":trackPeak"), QVariant());
        }

        query.bindValue(qTEXT(":dateTime"), track_info.last_write_time);
        query.bindValue(qTEXT(":genre"), toQString(track_info.genre));
        query.bindValue(qTEXT(":comment"), toQString(track_info.comment));

        query.bindValue(qTEXT(":musicId"), music_id);

        DbIfFailedThrow1(query);
    }

    void MusicDao::updateMusicFilePath(int32_t music_id, const QString& file_path) {
        SqlQuery query(db_);

        query.prepare(qTEXT("UPDATE musics SET path = :path WHERE (musicId = :musicId)"));

        query.bindValue(qTEXT(":musicId"), music_id);
        query.bindValue(qTEXT(":path"), file_path);

        DbIfFailedThrow1(query);
    }

    void MusicDao::addOrUpdateLyrics(int32_t music_id, const QString& lyrc, const QString& trlyrc) {
        SqlQuery query(db_);

        query.prepare(qTEXT("UPDATE musics SET lyrc = :lyrc, trlyrc = :trlyrc WHERE (musicId = :musicId)"));

        query.bindValue(qTEXT(":musicId"), music_id);
        query.bindValue(qTEXT(":lyrc"), lyrc);
        query.bindValue(qTEXT(":trlyrc"), trlyrc);

        DbIfFailedThrow1(query);
    }

    std::optional<std::tuple<QString, QString>> MusicDao::getLyrics(int32_t music_id) {
        SqlQuery query(db_);
        query.prepare(qTEXT(R"(
    SELECT
        lyrc, trLyrc
    FROM
        musics
    WHERE
        musicId = :musicId
    )")
        );

        query.bindValue(qTEXT(":musicId"), music_id);

        DbIfFailedThrow1(query);

        while (query.next()) {
            auto lyrc = query.value(qTEXT("lyrc")).toString();
            auto tr_lyrc = query.value(qTEXT("trLyrc")).toString();
            if (!lyrc.isEmpty()) {
                return std::optional<std::tuple<QString, QString>> { std::in_place_t{},
                    std::tuple<QString, QString> { lyrc, tr_lyrc } };
            }
        }
        return std::nullopt;
    }

    void MusicDao::updateMusicRating(int32_t music_id, int32_t rating) {
        SqlQuery query(db_);

        query.prepare(qTEXT("UPDATE musics SET rating = :rating WHERE (musicId = :musicId)"));

        query.bindValue(qTEXT(":musicId"), music_id);
        query.bindValue(qTEXT(":rating"), rating);

        DbIfFailedThrow1(query);
    }

    void MusicDao::updateMusicHeart(int32_t music_id, uint32_t heart) {
        SqlQuery query(db_);

        query.prepare(qTEXT("UPDATE musics SET heart = :heart WHERE (musicId = :musicId)"));

        query.bindValue(qTEXT(":musicId"), music_id);
        query.bindValue(qTEXT(":heart"), heart);

        DbIfFailedThrow1(query);
    }

    void MusicDao::updateMusicTitle(int32_t music_id, const QString& title) {
        SqlQuery query(db_);

        query.prepare(qTEXT("UPDATE musics SET title = :title WHERE (musicId = :musicId)"));

        query.bindValue(qTEXT(":musicId"), music_id);
        query.bindValue(qTEXT(":title"), title);

        DbIfFailedThrow1(query);
    }

    void MusicDao::updateMusicPlays(int32_t music_id) {
        SqlQuery query(db_);

        query.prepare(qTEXT("UPDATE musics SET plays = plays + 1 WHERE (musicId = :musicId)"));
        query.bindValue(qTEXT(":musicId"), music_id);
        DbIfFailedThrow1(query);
    }

    QString MusicDao::getMusicCoverId(int32_t music_id) const {
        SqlQuery query(db_);

        query.prepare(qTEXT("SELECT coverId FROM musics WHERE musicId = (:musicId)"));
        query.bindValue(qTEXT(":musicId"), music_id);

        DbIfFailedThrow1(query);

        const auto index = query.record().indexOf(qTEXT("coverId"));
        if (query.next()) {
            return query.value(index).toString();
        }
        return kEmptyString;
    }

    QString MusicDao::getMusicFilePath(int32_t music_id) const {
        SqlQuery query(db_);

        query.prepare(qTEXT("SELECT path FROM musics WHERE musicId = (:musicId)"));
        query.bindValue(qTEXT(":musicId"), music_id);

        DbIfFailedThrow1(query);

        const auto index = query.record().indexOf(qTEXT("path"));
        if (query.next()) {
            return query.value(index).toString();
        }
        return kEmptyString;
    }

    void MusicDao::setMusicCover(int32_t music_id, const QString& cover_id) {
        XAMP_ENSURES(!cover_id.isEmpty());

        SqlQuery query(db_);

        query.prepare(qTEXT("UPDATE musics SET coverId = :coverId WHERE (musicId = :musicId)"));

        query.bindValue(qTEXT(":musicId"), music_id);
        query.bindValue(qTEXT(":coverId"), cover_id);

        DbIfFailedThrow1(query);
    }

    void MusicDao::removeMusic(int32_t music_id) const {
        SqlQuery query(db_);
        query.prepare(qTEXT("DELETE FROM musics WHERE musicId=:musicId"));
        query.bindValue(qTEXT(":musicId"), music_id);
        DbIfFailedThrow1(query);
    }

    void MusicDao::removeTrackLoudnessMusicId(int32_t music_id) {
        SqlQuery query(db_);
        query.prepare(qTEXT("DELETE FROM musicLoudness WHERE musicId=:musicId"));
        query.bindValue(qTEXT(":musicId"), music_id);
        DbIfFailedThrow1(query);
    }

    void MusicDao::removeMusic(const QString& file_path) {
        SqlQuery query(db_);

        PlaylistDao playlist(db_);

        query.prepare(qTEXT("SELECT musicId FROM musics WHERE path = (:path)"));
        query.bindValue(qTEXT(":path"), file_path);

        query.exec();

        if (query.next()) {
            const auto music_id = query.value(qTEXT("musicId")).toInt();
            playlist.removePlaylistMusic(1, QVector<int32_t>{ music_id });
            //removeAlbumMusicId(music_id);
            removeTrackLoudnessMusicId(music_id);
            //removeAlbumArtistId(music_id);
            removeMusic(music_id);
            return;
        }
    }

    void MusicDao::addOrUpdateMusicLoudness(int32_t album_id, int32_t artist_id, int32_t music_id, double track_loudness) const {
        SqlQuery query(db_);

        query.prepare(qTEXT(R"(
        INSERT OR REPLACE INTO musicLoudness (albumMusicId, albumId, artistId, musicId, trackLoudness)
        VALUES ((SELECT albumMusicId from musicLoudness where albumId = :albumId AND artistId = :artistId AND musicId = :musicId), :albumId, :artistId, :musicId, :trackLoudness)
        )"));

        query.bindValue(qTEXT(":albumId"), album_id);
        query.bindValue(qTEXT(":artistId"), artist_id);
        query.bindValue(qTEXT(":musicId"), music_id);
        query.bindValue(qTEXT(":trackLoudness"), track_loudness);

        DbIfFailedThrow1(query);
    }
}
