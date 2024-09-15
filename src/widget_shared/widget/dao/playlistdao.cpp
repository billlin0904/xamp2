#include <QSqlTableModel>

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
}
