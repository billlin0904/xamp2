#include <QDir>
#include <QFileInfo>
#include <QVariant>
#include <widget/playlistentity.h>

#include <widget/playlisttablemodel.h>

namespace {
    void setEntityValue(std::optional<double>& opt_value, const QVariant& db_value) {
        if (!db_value.isNull()) {
            opt_value = db_value.toDouble();
        }
    }
}

QVariant indexValue(const QModelIndex& index, const QModelIndex& src, int i) {
    return index.model()->data(index.model()->index(src.row(), i));
}

QVariant indexValue(const QModelIndex& index, int i) {
    return index.model()->data(index.model()->index(index.row(), i));
}

PlayListEntity getEntity(const QModelIndex& index) {
    PlayListEntity entity;
    entity.music_id          = indexValue(index, PLAYLIST_MUSIC_ID).toInt();
    entity.playing           = indexValue(index, PLAYLIST_IS_PLAYING).toInt();
    entity.track             = indexValue(index, PLAYLIST_TRACK).toUInt();
    entity.file_path         = indexValue(index, PLAYLIST_FILE_PATH).toString();
    entity.file_size         = indexValue(index, PLAYLIST_FILE_SIZE).toULongLong();
    entity.title             = indexValue(index, PLAYLIST_TITLE).toString();
    entity.artist            = indexValue(index, PLAYLIST_ARTIST).toString();
    entity.album             = indexValue(index, PLAYLIST_ALBUM).toString();
    entity.duration          = indexValue(index, PLAYLIST_DURATION).toDouble();
    entity.bit_rate          = indexValue(index, PLAYLIST_BIT_RATE).toUInt();
    entity.sample_rate       = indexValue(index, PLAYLIST_SAMPLE_RATE).toUInt();
    entity.album_id          = indexValue(index, PLAYLIST_ALBUM_ID).toInt();
    entity.artist_id         = indexValue(index, PLAYLIST_ARTIST_ID).toInt();
    entity.cover_id          = indexValue(index, PLAYLIST_ALBUM_COVER_ID).toString();
    entity.timestamp         = indexValue(index, PLAYLIST_LAST_UPDATE_TIME).toULongLong();
    entity.playlist_music_id = indexValue(index, PLAYLIST_PLAYLIST_MUSIC_ID).toInt();
    entity.genre             = indexValue(index, PLAYLIST_GENRE).toString();
    entity.heart             = indexValue(index, PLAYLIST_LIKE).toUInt();
    entity.comment           = indexValue(index, PLAYLIST_COMMENT).toString();
    entity.year              = indexValue(index, PLAYLIST_YEAR).toUInt();
    entity.music_cover_id    = indexValue(index, PLAYLIST_MUSIC_COVER_ID).toString();
	entity.is_cue_file       = indexValue(index, PLAYLIST_IS_CUE_FILE).toBool();

    setEntityValue(entity.offset, indexValue(index, PLAYLIST_OFFSET));

    setEntityValue(entity.album_replay_gain,  indexValue(index, PLAYLIST_ALBUM_RG));
    setEntityValue(entity.album_peak,         indexValue(index, PLAYLIST_ALBUM_PK));
    setEntityValue(entity.track_replay_gain,  indexValue(index, PLAYLIST_TRACK_RG));
    setEntityValue(entity.track_peak,         indexValue(index, PLAYLIST_TRACK_PK));
    setEntityValue(entity.track_loudness,     indexValue(index, PLAYLIST_TRACK_LOUDNESS));

    entity.yt_music_album_id = indexValue(index, PLAYLIST_YT_MUSIC_ALBUM_ID).toString();
    entity.yt_music_artist_id = indexValue(index, PLAYLIST_YT_MUSIC_ARIST_ID).toString();

    const QFileInfo file_info(entity.file_path);
    entity.file_extension = file_info.suffix();
    entity.file_name      = file_info.completeBaseName();
    entity.parent_path    = toNativeSeparators(file_info.dir().path());

    if (entity.file_extension.isEmpty()) {
        entity.file_extension = ".opus"_str;
    }

    return entity;
}
