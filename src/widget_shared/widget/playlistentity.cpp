#include <widget/playlistentity.h>

#include <widget/playlisttablemodel.h>

QVariant GetIndexValue(const QModelIndex& index, const QModelIndex& src, int i) {
    return index.model()->data(index.model()->index(src.row(), i));
}

QVariant GetIndexValue(const QModelIndex& index, int i) {
    return index.model()->data(index.model()->index(index.row(), i));
}

PlayListEntity GetEntity(const QModelIndex& index) {
    PlayListEntity entity;
    entity.music_id = GetIndexValue(index, PLAYLIST_MUSIC_ID).toInt();
    entity.playing = GetIndexValue(index, PLAYLIST_PLAYING).toInt();
    entity.track = GetIndexValue(index, PLAYLIST_TRACK).toUInt();
    entity.file_path = GetIndexValue(index, PLAYLIST_FILE_PATH).toString();
    entity.file_size = GetIndexValue(index, PLAYLIST_FILE_SIZE).toULongLong();
    entity.title = GetIndexValue(index, PLAYLIST_TITLE).toString();
    entity.file_name = GetIndexValue(index, PLAYLIST_FILE_NAME).toString();
    entity.artist = GetIndexValue(index, PLAYLIST_ARTIST).toString();
    entity.album = GetIndexValue(index, PLAYLIST_ALBUM).toString();
    entity.duration = GetIndexValue(index, PLAYLIST_DURATION).toDouble();
    entity.bit_rate = GetIndexValue(index, PLAYLIST_BIT_RATE).toUInt();
    entity.sample_rate = GetIndexValue(index, PLAYLIST_SAMPLE_RATE).toUInt();
    entity.album_id = GetIndexValue(index, PLAYLIST_ALBUM_ID).toInt();
    entity.artist_id = GetIndexValue(index, PLAYLIST_ARTIST_ID).toInt();
    entity.cover_id = GetIndexValue(index, PLAYLIST_COVER_ID).toString();
    entity.file_extension = GetIndexValue(index, PLAYLIST_FILE_EXT).toString();
    entity.parent_path = GetIndexValue(index, PLAYLIST_FILE_PARENT_PATH).toString();
    entity.timestamp = GetIndexValue(index, PLAYLIST_LAST_UPDATE_TIME).toULongLong();
    entity.playlist_music_id = GetIndexValue(index, PLAYLIST_PLAYLIST_MUSIC_ID).toInt();
    entity.album_replay_gain = GetIndexValue(index, PLAYLIST_ALBUM_RG).toDouble();
    entity.album_peak = GetIndexValue(index, PLAYLIST_ALBUM_PK).toDouble();
    entity.track_replay_gain = GetIndexValue(index, PLAYLIST_TRACK_RG).toDouble();
    entity.track_peak = GetIndexValue(index, PLAYLIST_TRACK_PK).toDouble();
    entity.track_loudness = GetIndexValue(index, PLAYLIST_TRACK_LOUDNESS).toDouble();
    entity.genre = GetIndexValue(index, PLAYLIST_GENRE).toString();
    entity.heart = GetIndexValue(index, PLAYLIST_HEART).toUInt();
    return entity;
}
