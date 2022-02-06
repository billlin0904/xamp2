#include <widget/albumentity.h>

QVariant getIndexValue(const QModelIndex& index, int i) {
    return index.model()->data(index.model()->index(index.row(), i));
}

AlbumEntity getAlbumEntity(const QModelIndex& index) {
    auto title = getIndexValue(index, 1).toString();
    auto musicId = getIndexValue(index, 3).toInt();
    auto artist = getIndexValue(index, 4).toString();
    auto file_ext = getIndexValue(index, 5).toString();
    auto file_path = getIndexValue(index, 6).toString();
    auto cover_id = getIndexValue(index, 7).toString();
    auto album = getIndexValue(index, 8).toString();

    auto artistId = getIndexValue(index, 9).toInt();
    auto albumId = getIndexValue(index, 10).toInt();

    AlbumEntity entity;

    entity.music_id = musicId;
    entity.album = album;
    entity.title = title;
    entity.artist = artist;
    entity.cover_id = cover_id;
    entity.file_path = file_path;
    entity.file_ext = file_ext;
    entity.album_id = albumId;
    entity.artist_id = artistId;

    return entity;
}

AlbumEntity toAlbumEntity(const PlayListEntity& item) {
    AlbumEntity music_entity;
    music_entity.music_id = item.music_id;
    music_entity.artist_id = item.artist_id;
    music_entity.album_id = item.album_id;
    music_entity.cover_id = item.cover_id;
    music_entity.album_replay_gain = item.album_replay_gain;
    music_entity.track_replay_gain = item.track_replay_gain;
    music_entity.title = item.title;
    music_entity.album = item.album;
    music_entity.artist = item.artist;
    music_entity.file_ext = item.file_ext;
    music_entity.file_path = item.file_path;
    return music_entity;
}
