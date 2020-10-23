#include <QSqlQuery>

#include <widget/str_utilts.h>
#include <widget/playlisttablequerymodel.h>

PlayListTableQueryModel::PlayListTableQueryModel(int32_t playlist_id, QObject* parent)
	: QSqlQueryModel(parent)
	, playlist_id_(playlist_id) {
}

Qt::ItemFlags PlayListTableQueryModel::flags(const QModelIndex& index) const {
	/*auto col = index.column();
	if (col == CHECKBOX_ROW) {
		return QSqlQueryModel::flags(index) | Qt::ItemIsUserCheckable;
	}*/
	return QSqlQueryModel::flags(index);
}

void PlayListTableQueryModel::selectAll(bool check) {
	/*for (auto i = 0; i < rowCount(); ++i) {
		auto idx = index(i, 0);
		updateSelected(idx, check);
	}
	dynamic_cast<PlaybackHistoryTableView*>(parent())->refreshOnece();*/
}

void PlayListTableQueryModel::updateSelected(const QModelIndex& index, bool check) {
	/*QSqlQuery query;
	auto id = getIndexValue(index, HISTORY_ID_ROW).toInt();
	query.prepare(Q_UTF8("UPDATE playbackHistory SET selected = ? WHERE playbackHistoryId = ?"));
	query.addBindValue(check);
	query.addBindValue(id);
	query.exec();*/
}

bool PlayListTableQueryModel::setData(const QModelIndex& index, const QVariant& value, int role) {
	/*if (index.column() == CHECKBOX_ROW && role == Qt::CheckStateRole) {
		auto check = value == Qt::Checked ? 1 : 0;
		updateSelected(index, check);
		dynamic_cast<PlaybackHistoryTableView*>(parent())->refreshOnece();
		return QAbstractItemModel::setData(index, check);
	}*/
	return QSqlQueryModel::setData(index, value, role);
}

QVariant PlayListTableQueryModel::data(const QModelIndex& index, int role) const {
	auto col = index.column();
	/*if (col == CHECKBOX_ROW) {
		if (role == Qt::CheckStateRole) {
			int checked = QSqlQueryModel::data(index).toInt();
			if (checked) {
				return Qt::Checked;
			}
			else {
				return Qt::Unchecked;
			}
		}
		else {
			return QVariant();
		}
	}*/
	return QSqlQueryModel::data(index, role);
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
