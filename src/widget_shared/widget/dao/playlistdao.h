//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QString>
#include <widget/widget_shared.h>
#include <widget/database.h>

struct PlayListEntity;

namespace dao {

struct XAMP_WIDGET_SHARED_EXPORT PlaylistAlbumStats {
    int32_t album_count{ 0 };
    int32_t music_count{ 0 };
    double total_duration{0};
};

class XAMP_WIDGET_SHARED_EXPORT PlaylistDao final {
public:
    PlaylistDao();

    explicit PlaylistDao(QSqlDatabase& db);

    int32_t addPlaylist(const QString& name, int32_t play_index, StoreType store_type, const QString& cloud_playlist_id = kEmptyString);
    void setPlaylistName(int32_t playlist_id, const QString& name);
    void removePlaylist(int32_t playlist_id);
    void removePlaylistAllMusic(int32_t playlist_id);
    void updatePlaylistMusicChecked(int32_t playlist_music_id, bool is_checked);
    int32_t removePlaylistMusic(int32_t playlist_id, const QVector<int>& select_music_ids);
    bool isPlaylistExist(int32_t playlist_id) const;
    void setPlaylistIndex(int32_t playlist_id, int32_t play_index, StoreType store_type);
    void addMusicToPlaylist(int32_t music_id, int32_t playlist_id, int32_t album_id) const;
    void addMusicToPlaylist(const QList<int32_t>& music_id, int32_t playlist_id) const;
    void setNowPlaying(int32_t playlist_id, int32_t playlist_music_id);
    void clearNowPlaying(int32_t playlist_id);
    void clearNowPlaying(int32_t playlist_id, int32_t playlist_music_id);
    void clearNowPlayingSkipMusicId(int32_t playlist_id, int32_t skip_playlist_music_id);
    void setNowPlayingState(int32_t playlist_id, int32_t playlist_music_id, PlayingState playing);
    std::map<int32_t, int32_t> getPlaylistIndex(StoreType type);
    void forEachPlaylist(std::function<void(int32_t, int32_t, StoreType, QString, QString)>&& fun);
	QList<QString> getAlbumCoverIds(int32_t playlist_id);
    PlaylistAlbumStats getAlbumStats(int32_t playlist_id);
	std::pair<QVariant, QVariant> getPlaylistMusic(int32_t playlist_id, int32_t playlist_music_id);
    void swapPlaylistMusicId(int32_t playlist_id, const PlayListEntity& music_entity_1, const PlayListEntity& music_entity_2);
    void updatePlaylistMusic(int32_t playlist_musics_id, int32_t new_music_id, const QVariant& albumId, const QVariant& playing, const QVariant& is_checked);
private:
    QSqlDatabase& db_;
};

}