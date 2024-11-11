#include <QSqlTableModel>
#include <base/rng.h>
#include <widget/widget_shared.h>
#include <widget/dao/playlistdao.h>

namespace dao {
    PlaylistDao::PlaylistDao()
        : PlaylistDao(qGuiDb.getDatabase()) {
    }

	PlaylistDao::PlaylistDao(QSqlDatabase& db)
		: db_(db) {
	}

	int32_t PlaylistDao::addPlaylist(const QString& name, int32_t play_index, StoreType store_type, const QString& cloud_playlist_id) {
        QSqlTableModel model(nullptr, db_);

        model.setEditStrategy(QSqlTableModel::OnManualSubmit);
        model.setTable("playlist"_str);
        model.select();

        if (!model.insertRows(0, 1)) {
            return kInvalidDatabaseId;
        }

        model.setData(model.index(0, 0), QVariant());
        model.setData(model.index(0, 1), play_index);
        model.setData(model.index(0, 2), static_cast<int32_t>(store_type));
        model.setData(model.index(0, 3), cloud_playlist_id);
        model.setData(model.index(0, 4), name);

        if (!model.submitAll()) {
            return kInvalidDatabaseId;
        }

        model.database().commit();
        return model.query().lastInsertId().toInt();
	}

    void PlaylistDao::setPlaylistName(int32_t playlist_id, const QString& name) {
        SqlQuery query(db_);

        query.prepare("UPDATE playlist SET name = :name WHERE (playlistId = :playlistId)"_str);

        query.bindValue(":playlistId"_str, playlist_id);
        query.bindValue(":name"_str, name);
        DbIfFailedThrow1(query);
    }

    void PlaylistDao::removePlaylist(int32_t playlist_id) {
        SqlQuery query(db_);
        query.prepare("DELETE FROM playlist WHERE playlistId=:playlistId"_str);
        query.bindValue(":playlistId"_str, playlist_id);
        DbIfFailedThrow1(query);
    }

    void PlaylistDao::removePlaylistAllMusic(int32_t playlist_id) {
        SqlQuery query(db_);
        query.prepare("DELETE FROM playlistMusics WHERE playlistId=:playlistId"_str);
        query.bindValue(":playlistId"_str, playlist_id);
        DbIfFailedThrow1(query);
    }

    void PlaylistDao::updatePlaylistMusicChecked(int32_t playlist_music_id, bool is_checked) {
        SqlQuery query(db_);

        query.prepare("UPDATE playlistMusics SET isChecked = :isChecked WHERE (playlistMusicsId = :playlistMusicsId)"_str);

        query.bindValue(":playlistMusicsId"_str, playlist_music_id);
        query.bindValue(":isChecked"_str, is_checked);

        DbIfFailedThrow1(query);
    }

    int32_t PlaylistDao::removePlaylistMusic(int32_t playlist_id, const QVector<int>& select_music_ids) {
        SqlQuery query(db_);

        QString str = "DELETE FROM playlistMusics WHERE playlistId=:playlistId AND musicId in (%0)"_str;

        QStringList list;
        for (auto id : select_music_ids) {
            list << QString::number(id);
        }

        auto q = str.arg(list.join(","_str));
        query.prepare(q);

        query.bindValue(":playlistId"_str, playlist_id);
        DbIfFailedThrow1(query);

        auto affected_rows = 0;
        if (query.next()) {
            affected_rows = 1;
        }
        return affected_rows;
    }

    bool PlaylistDao::isPlaylistExist(int32_t playlist_id) const {
        SqlQuery query(db_);

        query.prepare("SELECT playlistId FROM playlist WHERE playlistId = (:playlistId)"_str);
        query.bindValue(":playlistId"_str, playlist_id);

        DbIfFailedThrow1(query);
        return query.next();
    }

    void PlaylistDao::setPlaylistIndex(int32_t playlist_id, int32_t play_index, StoreType store_type) {
        SqlQuery query(db_);

        query.prepare("UPDATE playlist SET playlistIndex = :playlistIndex WHERE (playlistId = :playlistId)"_str);

        query.bindValue(":playlistId"_str, playlist_id);
        query.bindValue(":playlistIndex"_str, play_index);
        query.bindValue(":storeType"_str, static_cast<int32_t>(store_type));
        DbIfFailedThrow1(query);
    }

    void PlaylistDao::addMusicToPlaylist(int32_t music_id, int32_t playlist_id, int32_t album_id) const {
        SqlQuery query(db_);

        const auto querystr = qFormat("INSERT INTO playlistMusics (playlistMusicsId, playlistId, musicId, albumId) VALUES (NULL, %1, %2, %3)")
            .arg(playlist_id)
            .arg(music_id)
            .arg(album_id);

        query.prepare(querystr);
        DbIfFailedThrow1(query);
    }

    void PlaylistDao::addMusicToPlaylist(const QList<int32_t>& music_id, int32_t playlist_id) const {
        if (music_id.isEmpty()) {
            return;
        }

        SqlQuery query(db_);

        QStringList strings;

        for (const auto id : music_id) {
            strings << "("_str + "NULL, "_str + QString::number(playlist_id) + ", "_str + QString::number(id) + ")"_str;
        }

        const auto querystr = "INSERT INTO playlistMusics (playlistMusicsId, playlistId, musicId) VALUES "_str
            + strings.join(","_str);
        query.prepare(querystr);
        DbIfFailedThrow1(query);
    }

    void PlaylistDao::setNowPlaying(int32_t playlist_id, int32_t playlist_music_id) {
        setNowPlayingState(playlist_id, playlist_music_id, PlayingState::PLAY_PLAYING);
    }
    
    void PlaylistDao::clearNowPlaying(int32_t playlist_id) {
      SqlQuery query(db_);
      query.prepare("UPDATE playlistMusics SET playing = :playing"_str);
      query.bindValue(":playing"_str, PlayingState::PLAY_CLEAR);
      query.bindValue(":playlistId"_str, playlist_id);
      DbIfFailedThrow1(query);
    }
    
    void PlaylistDao::clearNowPlaying(int32_t playlist_id, int32_t playlist_music_id) {
        setNowPlayingState(playlist_id, playlist_music_id, PlayingState::PLAY_CLEAR);
    }

    void PlaylistDao::clearNowPlayingSkipMusicId(int32_t playlist_id, int32_t skip_playlist_music_id) {
        SqlQuery query(db_);
        query.prepare("UPDATE playlistMusics SET playing = :playing WHERE (playlistMusicsId != :skipPlaylistMusicsId)"_str);
        query.bindValue(":playing"_str, PlayingState::PLAY_CLEAR);
        query.bindValue(":playlistId"_str, playlist_id);
        query.bindValue(":skipPlaylistMusicsId"_str, skip_playlist_music_id);
        DbIfFailedThrow1(query);
    }

    void PlaylistDao::setNowPlayingState(int32_t playlist_id, int32_t playlist_music_id, PlayingState playing) {
        SqlQuery query(db_);
        query.prepare("UPDATE playlistMusics SET playing = :playing WHERE (playlistId = :playlistId AND playlistMusicsId = :playlistMusicsId)"_str);
        query.bindValue(":playing"_str, playing);
        query.bindValue(":playlistId"_str, playlist_id);
        query.bindValue(":playlistMusicsId"_str, playlist_music_id);
        DbIfFailedThrow1(query);
    }

    void PlaylistDao::updatePlaylistMusic(int32_t playlist_musics_id, int32_t new_music_id, const QVariant& albumId, const QVariant& playing, const QVariant& is_checked) {
        SqlQuery query(db_);
        query.prepare(R"(
        UPDATE playlistMusics 
        SET musicId = :new_music_id, albumId = :albumId, playing = :playing, isChecked = :isChecked 
        WHERE playlistMusicsId = :playlistMusicsId
    )"_str);
        query.bindValue(":playlistMusicsId"_str, playlist_musics_id);
        query.bindValue(":new_music_id"_str, new_music_id);
        query.bindValue(":albumId"_str, albumId);
        query.bindValue(":playing"_str, playing);
        query.bindValue(":isChecked"_str, is_checked);
        DbIfFailedThrow1(query);
    }

    std::pair<QVariant, QVariant> PlaylistDao::getPlaylistMusic(int32_t playlist_id, int32_t playlist_music_id) {
        SqlQuery query(db_);

        query.prepare(R"(
        SELECT playing, isChecked
		FROM playlistMusics
		WHERE playlistId = :playlist_id AND (playlistMusicsId = :playlist_music_id)
		)"_str);

        query.bindValue(":playlist_id"_str, playlist_id);
        query.bindValue(":playlist_music_id"_str, playlist_music_id);

        DbIfFailedThrow1(query);

        QVariant playing;
        QVariant is_checked;

        if (query.next()) {
            playing = query.value("playing"_str);
            is_checked = query.value("isChecked"_str);
        }
		return std::make_pair(playing, is_checked);
    }

    void PlaylistDao::swapPlaylistMusicId(int32_t playlist_id, const PlayListEntity& music_entity_1, const PlayListEntity& music_entity_2) {
        auto [playing1, is_checked1] = getPlaylistMusic(playlist_id, music_entity_1.playlist_music_id);
        auto [playing2, is_checked2] = getPlaylistMusic(playlist_id, music_entity_2.playlist_music_id);

        updatePlaylistMusic(music_entity_1.playlist_music_id, music_entity_2.music_id, music_entity_2.album_id, playing2, is_checked2);
        updatePlaylistMusic(music_entity_2.playlist_music_id, music_entity_1.music_id, music_entity_1.album_id, playing1, is_checked1);
    }

    std::map<int32_t, int32_t> PlaylistDao::getPlaylistIndex(StoreType type) {
        std::map<int32_t, int32_t> playlist_index;

        forEachPlaylist([&playlist_index, type](auto id, auto index, auto store_type, auto name, auto) {
            if (type == store_type) {
                playlist_index.insert(std::make_pair(index, id));
            }
            });
        return playlist_index;
    }

    void PlaylistDao::forEachPlaylist(std::function<void(int32_t, int32_t, StoreType, QString, QString)>&& fun) {
        QSqlTableModel model(nullptr, db_);

        model.setTable("playlist"_str);
        model.setSort(1, Qt::AscendingOrder);
        model.select();

        for (auto i = 0; i < model.rowCount(); ++i) {
            auto record = model.record(i);
            fun(record.value("playlistId"_str).toInt(),
                record.value("playlistIndex"_str).toInt(),
                static_cast<StoreType>(record.value("storeType"_str).toInt()),
                record.value("cloudPlaylistId"_str).toString(),
                record.value("name"_str).toString());
        }
    }

    QList<QString> PlaylistDao::getAlbumCoverIds(int32_t playlist_id) {
		SqlQuery query(db_);
		query.prepare(R"(SELECT DISTINCT a.coverId
            FROM playlistMusics pm
            JOIN albumMusic am ON pm.musicId = am.musicId
            JOIN albums a ON am.albumId = a.albumId
            WHERE pm.playlistId = :playlistId
            LIMIT 4;)"_str);
        query.bindValue(":playlistId"_str, playlist_id);
        DbIfFailedThrow1(query);

        QList<QString> covers;
        while (query.next()) {
            covers.push_back(query.value("coverId"_str).toString());
        }
        return covers;
    }

    PlaylistAlbumStats PlaylistDao::getAlbumStats(int32_t playlist_id) {
        SqlQuery query(db_);
        query.prepare(R"(
		SELECT
			COUNT( DISTINCT am.albumId ) AS album_count,
			COUNT( pm.musicId ) AS music_count,
			COALESCE( SUM( m.duration ), 0 ) AS total_duration 
		FROM
			playlistMusics pm
			JOIN musics m ON pm.musicId = m.musicId
			JOIN albumMusic am ON m.musicId = am.musicId 
		WHERE
			pm.playlistId = :playlistId;
		)"_str);
        query.bindValue(":playlistId"_str, playlist_id);
        DbIfFailedThrow1(query);

        PlaylistAlbumStats stats;
        if (query.next()) {
            stats.album_count = query.value("album_count"_str).toInt();
            stats.music_count = query.value("music_count"_str).toInt();
            stats.total_duration = query.value("total_duration"_str).toDouble();
        }
        return stats;
    }
}
