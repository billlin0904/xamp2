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

        query.prepare(R"(
    INSERT OR REPLACE INTO musics
    (musicId, title, track, path, fileExt, fileName, duration, durationStr, parentPath, bitRate, sampleRate, offset, dateTime, albumReplayGain, trackReplayGain, albumPeak, trackPeak, genre, comment, fileSize, heart, isCueFile)
    VALUES ((SELECT musicId FROM musics WHERE path = :path AND durationStr = :durationStr), :title, :track, :path, :fileExt, :fileName, :duration, :durationStr, :parentPath, :bitRate, :sampleRate, :offset, :dateTime, :albumReplayGain, :trackReplayGain, :albumPeak, :trackPeak, :genre, :comment, :fileSize, :heart, :isCueFile)
    )"_str
        );

        query.bindValue(":title"_str, toQString(track_info.title));
        query.bindValue(":track"_str, track_info.track);
        query.bindValue(":path"_str, toQString(track_info.file_path));
        query.bindValue(":fileExt"_str, optStrToQString(track_info.file_ext()));
        query.bindValue(":fileName"_str, optStrToQString(track_info.file_name()));
        query.bindValue(":parentPath"_str, optStrToQString(track_info.parent_path()));
        query.bindValue(":duration"_str, track_info.duration);
        query.bindValue(":durationStr"_str, formatDuration(track_info.duration));
        query.bindValue(":bitRate"_str, track_info.bit_rate);
        query.bindValue(":sampleRate"_str, track_info.sample_rate);
        query.bindValue(":offset"_str, track_info.offset);
        query.bindValue(":fileSize"_str, track_info.file_size);
        query.bindValue(":heart"_str, track_info.rating ? 1 : 0);
        query.bindValue(":isCueFile"_str, track_info.is_cue_file ? 1 : 0);

        if (track_info.replay_gain) {
            query.bindValue(":albumReplayGain"_str, track_info.replay_gain.value().album_gain);
            query.bindValue(":trackReplayGain"_str, track_info.replay_gain.value().track_gain);
            query.bindValue(":albumPeak"_str, track_info.replay_gain.value().album_peak);
            query.bindValue(":trackPeak"_str, track_info.replay_gain.value().track_peak);
        }
        else {
            query.bindValue(":albumReplayGain"_str, QVariant());
            query.bindValue(":trackReplayGain"_str, QVariant());
            query.bindValue(":albumPeak"_str, QVariant());
            query.bindValue(":trackPeak"_str, QVariant());
        }

        query.bindValue(":dateTime"_str, track_info.last_write_time);
        query.bindValue(":genre"_str, toQString(track_info.genre));
        query.bindValue(":comment"_str, toQString(track_info.comment));

        DbIfFailedThrow1(query);

        const auto music_id = query.lastInsertId().toInt();
        XAMP_ENSURES(music_id != kInvalidDatabaseId);
        return music_id;
    }

    void MusicDao::updateMusic(int32_t music_id, const TrackInfo& track_info) {
        SqlQuery query(db_);

        query.prepare(R"(
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
    )"_str
        );

        query.bindValue(":title"_str, toQString(track_info.title));
        query.bindValue(":track"_str, track_info.track);
        query.bindValue(":path"_str, toQString(track_info.file_path));
        query.bindValue(":fileExt"_str, optStrToQString(track_info.file_ext()));
        query.bindValue(":fileName"_str, optStrToQString(track_info.file_name()));
        query.bindValue(":parentPath"_str, optStrToQString(track_info.parent_path()));
        query.bindValue(":duration"_str, track_info.duration);
        query.bindValue(":durationStr"_str, formatDuration(track_info.duration));
        query.bindValue(":bitRate"_str, track_info.bit_rate);
        query.bindValue(":sampleRate"_str, track_info.sample_rate);
        query.bindValue(":offset"_str, track_info.offset);
        query.bindValue(":fileSize"_str, track_info.file_size);
        query.bindValue(":heart"_str, track_info.rating ? 1 : 0);

        if (track_info.replay_gain) {
            query.bindValue(":albumReplayGain"_str, track_info.replay_gain.value().album_gain);
            query.bindValue(":trackReplayGain"_str, track_info.replay_gain.value().track_gain);
            query.bindValue(":albumPeak"_str, track_info.replay_gain.value().album_peak);
            query.bindValue(":trackPeak"_str, track_info.replay_gain.value().track_peak);
        }
        else {
            query.bindValue(":albumReplayGain"_str, QVariant());
            query.bindValue(":trackReplayGain"_str, QVariant());
            query.bindValue(":albumPeak"_str, QVariant());
            query.bindValue(":trackPeak"_str, QVariant());
        }

        query.bindValue(":dateTime"_str, track_info.last_write_time);
        query.bindValue(":genre"_str, toQString(track_info.genre));
        query.bindValue(":comment"_str, toQString(track_info.comment));

        query.bindValue(":musicId"_str, music_id);

        DbIfFailedThrow1(query);
    }

    void MusicDao::updateMusicFilePath(int32_t music_id, const QString& file_path) {
        SqlQuery query(db_);

        query.prepare("UPDATE musics SET path = :path WHERE (musicId = :musicId)"_str);

        query.bindValue(":musicId"_str, music_id);
        query.bindValue(":path"_str, file_path);

        DbIfFailedThrow1(query);
    }

    void MusicDao::addOrUpdateLyrics(int32_t music_id, const QString& lyrc, const QString& trlyrc) {
        SqlQuery query(db_);

        query.prepare("UPDATE musics SET lyrc = :lyrc, trlyrc = :trlyrc WHERE (musicId = :musicId)"_str);

        query.bindValue(":musicId"_str, music_id);
        query.bindValue(":lyrc"_str, lyrc);
        query.bindValue(":trlyrc"_str, trlyrc);

        DbIfFailedThrow1(query);
    }

    std::optional<std::tuple<QString, QString>> MusicDao::getLyrics(int32_t music_id) {
        SqlQuery query(db_);
        query.prepare(R"(
    SELECT
        lyrc, trLyrc
    FROM
        musics
    WHERE
        musicId = :musicId
    )"_str
        );

        query.bindValue(":musicId"_str, music_id);

        DbIfFailedThrow1(query);

        while (query.next()) {
            auto lyrc = query.value("lyrc"_str).toString();
            auto tr_lyrc = query.value("trLyrc"_str).toString();
            if (!lyrc.isEmpty()) {
                return std::optional<std::tuple<QString, QString>> { std::in_place_t{},
                    std::tuple<QString, QString> { lyrc, tr_lyrc } };
            }
        }
        return std::nullopt;
    }

    void MusicDao::updateMusicRating(int32_t music_id, int32_t rating) {
        SqlQuery query(db_);

        query.prepare("UPDATE musics SET rating = :rating WHERE (musicId = :musicId)"_str);

        query.bindValue(":musicId"_str, music_id);
        query.bindValue(":rating"_str, rating);

        DbIfFailedThrow1(query);
    }

    void MusicDao::updateMusicHeart(int32_t music_id, uint32_t heart) {
        SqlQuery query(db_);

        query.prepare("UPDATE musics SET heart = :heart WHERE (musicId = :musicId)"_str);

        query.bindValue(":musicId"_str, music_id);
        query.bindValue(":heart"_str, heart);

        DbIfFailedThrow1(query);
    }

    void MusicDao::updateMusicTitle(int32_t music_id, const QString& title) {
        SqlQuery query(db_);

        query.prepare("UPDATE musics SET title = :title WHERE (musicId = :musicId)"_str);

        query.bindValue(":musicId"_str, music_id);
        query.bindValue(":title"_str, title);

        DbIfFailedThrow1(query);
    }

    void MusicDao::updateMusicPlays(int32_t music_id) {
        SqlQuery query(db_);

        query.prepare("UPDATE musics SET plays = plays + 1 WHERE (musicId = :musicId)"_str);
        query.bindValue(":musicId"_str, music_id);
        DbIfFailedThrow1(query);
    }

    QString MusicDao::getMusicCoverId(int32_t music_id) const {
        SqlQuery query(db_);

        query.prepare("SELECT coverId FROM musics WHERE musicId = (:musicId)"_str);
        query.bindValue(":musicId"_str, music_id);

        DbIfFailedThrow1(query);

        const auto index = query.record().indexOf("coverId"_str);
        if (query.next()) {
            return query.value(index).toString();
        }
        return kEmptyString;
    }

    QString MusicDao::getMusicFilePath(int32_t music_id) const {
        SqlQuery query(db_);

        query.prepare("SELECT path FROM musics WHERE musicId = (:musicId)"_str);
        query.bindValue(":musicId"_str, music_id);

        DbIfFailedThrow1(query);

        const auto index = query.record().indexOf("path"_str);
        if (query.next()) {
            return query.value(index).toString();
        }
        return kEmptyString;
    }

    void MusicDao::setMusicCover(int32_t music_id, const QString& cover_id) {
        XAMP_ENSURES(!cover_id.isEmpty());

        SqlQuery query(db_);

        query.prepare("UPDATE musics SET coverId = :coverId WHERE (musicId = :musicId)"_str);

        query.bindValue(":musicId"_str, music_id);
        query.bindValue(":coverId"_str, cover_id);

        DbIfFailedThrow1(query);
    }

    void MusicDao::removeMusic(int32_t music_id) const {
        SqlQuery query(db_);
        query.prepare("DELETE FROM musics WHERE musicId=:musicId"_str);
        query.bindValue(":musicId"_str, music_id);
        DbIfFailedThrow1(query);
    }

    void MusicDao::removeTrackLoudnessMusicId(int32_t music_id) {
        SqlQuery query(db_);
        query.prepare("DELETE FROM musicLoudness WHERE musicId=:musicId"_str);
        query.bindValue(":musicId"_str, music_id);
        DbIfFailedThrow1(query);
    }

    void MusicDao::removeMusic(const QString& file_path) {
        SqlQuery query(db_);

        PlaylistDao playlist(db_);

        query.prepare("SELECT musicId FROM musics WHERE path = (:path)"_str);
        query.bindValue(":path"_str, file_path);

        query.exec();

        if (query.next()) {
            const auto music_id = query.value("musicId"_str).toInt();
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

        query.prepare(R"(
        INSERT OR REPLACE INTO musicLoudness (albumMusicId, albumId, artistId, musicId, trackLoudness)
        VALUES ((SELECT albumMusicId from musicLoudness where albumId = :albumId AND artistId = :artistId AND musicId = :musicId), :albumId, :artistId, :musicId, :trackLoudness)
        )"_str);

        query.bindValue(":albumId"_str, album_id);
        query.bindValue(":artistId"_str, artist_id);
        query.bindValue(":musicId"_str, music_id);
        query.bindValue(":trackLoudness"_str, track_loudness);

        DbIfFailedThrow1(query);
    }
}
