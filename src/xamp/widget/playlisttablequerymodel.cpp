#include <QSqlQuery>

#include <widget/str_utilts.h>
#include <widget/playlisttablequerymodel.h>

PlayListTableQueryModel::PlayListTableQueryModel(int32_t playlist_id, QObject* parent)
	: QSqlQueryModel(parent)
	, playlist_id_(playlist_id) {
}

void PlayListTableQueryModel::refreshOnece() {
	QSqlQuery query(Q_UTF8(R"(
                             SELECT
                             albumMusic.albumId,
                             albumMusic.artistId,
                             albums.album,
                             albums.coverId,
                             artists.artist,
                             musics.*
                             FROM
                             playlistMusics
                             JOIN playlist ON playlist.playlistId = playlistMusics.playlistId
                             JOIN albumMusic ON playlistMusics.musicId = albumMusic.musicId
                             JOIN musics ON playlistMusics.musicId = musics.musicId
                             JOIN albums ON albumMusic.albumId = albums.albumId
                             JOIN artists ON albumMusic.artistId = artists.artistId
                             WHERE
                             playlistMusics.playlistId = :playlist_id;
                             )"));
	query.addBindValue(playlist_id_);
	setQuery(query);
}
