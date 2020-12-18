#include <thememanager.h>
#include <widget/playlisttablemodel.h>
#include <widget/musicentity.h>
#include <widget/str_utilts.h>
#include <widget/time_utilts.h>
#include <widget/playlistsqlquerytablemodel.h>

static PlayListEntity getEntity(const QModelIndex& index) {
    PlayListEntity enity;
    enity.music_id = getIndexValue(index, PLAYLIST_MUSIC_ID).toInt();
    enity.playing = getIndexValue(index, PLAYLIST_PLAYING).toBool();
    enity.track = getIndexValue(index, PLAYLIST_TRACK).toUInt();
    enity.file_path = getIndexValue(index, PLAYLIST_FILEPATH).toString();
    enity.title = getIndexValue(index, PLAYLIST_TITLE).toString();
    enity.file_name = getIndexValue(index, PLAYLIST_FILE_NAME).toString();
    enity.artist = getIndexValue(index, PLAYLIST_ARTIST).toString();
    enity.album = getIndexValue(index, PLAYLIST_ALBUM).toString();
    enity.duration = getIndexValue(index, PLAYLIST_DURATION).toDouble();
    enity.bitrate = getIndexValue(index, PLAYLIST_BITRATE).toUInt();
    enity.samplerate = getIndexValue(index, PLAYLIST_SAMPLE_RATE).toUInt();
    enity.rating = getIndexValue(index, PLAYLIST_RATING).toUInt();
    enity.album_id = getIndexValue(index, PLAYLIST_ALBUM_ID).toInt();
    enity.artist_id = getIndexValue(index, PLAYLIST_ARTIST_ID).toInt();
    enity.cover_id = getIndexValue(index, PLAYLIST_COVER_ID).toString();
    enity.fingerprint = getIndexValue(index, PLAYLIST_FINGER_PRINT).toString();
    return enity;
}

PlayListSqlQueryTableModel::PlayListSqlQueryTableModel(QObject *parent)
    : QSqlQueryModel(parent) {
}

QVariant PlayListSqlQueryTableModel::data(const QModelIndex& index, int32_t role) const {
    switch (role) {
    case Qt::DisplayRole:
        if (index.column() == PLAYLIST_PLAYING) {
            return QVariant();
        } else {
            auto value = QSqlQueryModel::data(index, Qt::DisplayRole);
            switch (index.column()) {
            case PLAYLIST_BITRATE:
                if (value.toInt() > 10000) {
                    return QString(Q_UTF8("%0 Mbps")).arg(value.toInt() / 1000.0);
                }
                return QString(Q_UTF8("%0 bps")).arg(value.toInt());
            case PLAYLIST_DURATION:
                return Time::msToString(value.toDouble());
            }
        }
        break;
    case Qt::DecorationRole:
    {
        if (index.column() == PLAYLIST_PLAYING) {
            auto value = QSqlQueryModel::data(index, Qt::DisplayRole);
            if (value.toBool()) {
                return ThemeManager::instance().playArrow();
            } else {
                return QVariant();
            }
        }
    }
        break;
    case Qt::TextAlignmentRole:
        switch (index.column()) {
        case PLAYLIST_DURATION:
            return Qt::AlignCenter;
        }
    default:
        break;
    }
    return QSqlQueryModel::data(index, role);
}
