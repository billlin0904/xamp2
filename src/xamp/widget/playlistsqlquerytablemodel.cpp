#include <thememanager.h>
#include <QDateTime>
#include <widget/playlisttablemodel.h>
#include <widget/albumentity.h>
#include <widget/str_utilts.h>
#include <widget/ui_utilts.h>
#include <widget/playlistsqlquerytablemodel.h>

PlayListSqlQueryTableModel::PlayListSqlQueryTableModel(QObject *parent)
    : QSqlQueryModel(parent) {
}

Qt::ItemFlags PlayListSqlQueryTableModel::flags(const QModelIndex& index) const {
    if (!index.isValid()) {
        return QAbstractTableModel::flags(index);
    }
    auto flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
    if (index.column() == PLAYLIST_RATING) {
        flags |= Qt::ItemIsEditable;
    }
    return flags;
}

QVariant PlayListSqlQueryTableModel::data(const QModelIndex& index, int32_t role) const {
    QVariant value;
    switch (role) {
    case Qt::FontRole:
        if (index.column() == PLAYLIST_DURATION
            || index.column() == PLAYLIST_TRACK
            || index.column() == PLAYLIST_BITRATE
            || index.column() == PLAYLIST_SAMPLE_RATE
            || index.column() == PLAYLIST_ALBUM_PK
            || index.column() == PLAYLIST_ALBUM_RG
            || index.column() == PLAYLIST_TRACK_PK
            || index.column() == PLAYLIST_TRACK_RG
            || index.column() == PLAYLIST_LAST_UPDATE_TIME) {
            return QFont(Q_UTF8("MonoFont"));
        }
        break;
    case Qt::DisplayRole:
	    {
		    if (index.column() == PLAYLIST_PLAYING) {
			    return {};
		    }
		    if (index.column() == PLAYLIST_RATING) {
			    value = QSqlQueryModel::data(index, Qt::DisplayRole);
			    return QVariant::fromValue(StarRating(value.toInt()));
		    }
		    value = QSqlQueryModel::data(index, Qt::DisplayRole);
		    switch (index.column()) {
		    case PLAYLIST_ALBUM_PK:
		    case PLAYLIST_ALBUM_RG:
            case PLAYLIST_TRACK_PK:
            case PLAYLIST_TRACK_RG:
                return QString::number(value.toFloat(), 'f', 2);
		    case PLAYLIST_BITRATE:
                return bitRate2String(value.toInt());
		    case PLAYLIST_SAMPLE_RATE:
			    return samplerate2String(value.toInt());
		    case PLAYLIST_DURATION:
			    return msToString(value.toDouble());
		    case PLAYLIST_LAST_UPDATE_TIME:
			    return QDateTime::fromSecsSinceEpoch(value.toULongLong()).toString(Q_UTF8("yyyy-MM-dd HH:mm:ss"));
		    }
	    }
        break;
    case Qt::DecorationRole:
        if (index.column() == PLAYLIST_PLAYING) {
            auto value = QSqlQueryModel::data(index, Qt::DisplayRole);
            if (value.toBool()) {
                return Singleton<ThemeManager>::GetInstance().playArrow();
            }
            return {};
        }
        break;
    case Qt::TextAlignmentRole:
        switch (index.column()) {
        case PLAYLIST_ALBUM_RG:
        case PLAYLIST_ALBUM_PK:
        case PLAYLIST_TRACK_RG:
        case PLAYLIST_TRACK_PK:
        case PLAYLIST_ARTIST:
        case PLAYLIST_DURATION:        
        case PLAYLIST_LAST_UPDATE_TIME:
            return static_cast<int>(Qt::AlignVCenter | Qt::AlignRight);
        case PLAYLIST_SAMPLE_RATE:
            return static_cast<int>(Qt::AlignVCenter | Qt::AlignHCenter);
        }
    default:
        break;
    }
    return QSqlQueryModel::data(index, role);
}
