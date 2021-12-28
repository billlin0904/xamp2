#include <thememanager.h>
#include <widget/playlisttablemodel.h>
#include <widget/musicentity.h>
#include <widget/str_utilts.h>
#include <widget/time_utilts.h>
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
            || index.column() == PLAYLIST_BITRATE
            || index.column() == PLAYLIST_SAMPLE_RATE
            || index.column() == PLAYLIST_ALBUM_PK
            || index.column() == PLAYLIST_ALBUM_RG
            || index.column() == PLAYLIST_TRACK_RG
            || index.column() == PLAYLIST_TIMESTAMP) {
            return QFont(Q_UTF8("MonoFont"));
        }
        break;
    case Qt::DisplayRole:
	    {
		    if (index.column() == PLAYLIST_PLAYING) {
			    return QVariant();
		    }
		    if (index.column() == PLAYLIST_RATING) {
			    value = QSqlQueryModel::data(index, Qt::DisplayRole);
			    return QVariant::fromValue(StarRating(value.toInt()));
		    }
		    value = QSqlQueryModel::data(index, Qt::DisplayRole);
		    switch (index.column()) {
		    case PLAYLIST_BITRATE:
			    if (value.toInt() > 10000) {
				    return QString(Q_UTF8("%0 Mbps")).arg(value.toInt() / 1000.0);
			    }
			    return QString(Q_UTF8("%0 Kbps")).arg(value.toInt());
		    case PLAYLIST_SAMPLE_RATE:
			    return samplerate2String(value.toInt());
		    case PLAYLIST_DURATION:
			    return Time::msToString(value.toDouble());
		    case PLAYLIST_TIMESTAMP:
			    return QDateTime::fromSecsSinceEpoch(value.toULongLong()).toString(Q_UTF8("yyyy-MM-dd HH:mm:ss"));
		    }
	    }
        break;
    case Qt::DecorationRole:
        if (index.column() == PLAYLIST_PLAYING) {
            auto value = QSqlQueryModel::data(index, Qt::DisplayRole);
            if (value.toBool()) {
                return ThemeManager::instance().playArrow();
            }
            return QVariant();
        }
        break;
    case Qt::TextAlignmentRole:
        switch (index.column()) {
        case PLAYLIST_ALBUM_RG:        
        case PLAYLIST_ARTIST:
        case PLAYLIST_DURATION:
        case PLAYLIST_ALBUM_PK:
        case PLAYLIST_TIMESTAMP:
            return QVariant(Qt::AlignVCenter | Qt::AlignRight);
        case PLAYLIST_SAMPLE_RATE:
            return QVariant(Qt::AlignVCenter | Qt::AlignHCenter);
        }
    default:
        break;
    }
    return QSqlQueryModel::data(index, role);
}
