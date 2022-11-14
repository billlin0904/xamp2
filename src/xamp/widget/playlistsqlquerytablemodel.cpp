#include <thememanager.h>
#include <QDateTime>
#include <widget/starrating.h>
#include <widget/playlisttablemodel.h>
#include <widget/albumentity.h>
#include <widget/database.h>
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
	switch (role) {
    case Qt::FontRole:
        /*if (index.column() == PLAYLIST_DURATION
            || index.column() == PLAYLIST_FILE_SIZE
            || index.column() == PLAYLIST_TRACK
            || index.column() == PLAYLIST_BIT_RATE
            || index.column() == PLAYLIST_SAMPLE_RATE
            || index.column() == PLAYLIST_ALBUM_PK
            || index.column() == PLAYLIST_ALBUM_RG
            || index.column() == PLAYLIST_TRACK_PK
            || index.column() == PLAYLIST_TRACK_RG
            || index.column() == PLAYLIST_LAST_UPDATE_TIME
            || index.column() == PLAYLIST_YEAR) {
            return QFont(Q_TEXT("MonoFont"));
        }*/
        break;
    case Qt::DisplayRole:
	    {
		    /*QVariant value;
		    if (index.column() == PLAYLIST_PLAYING) {
			    return {};
		    }
		    if (index.column() == PLAYLIST_RATING) {
			    value = QSqlQueryModel::data(index, Qt::DisplayRole);
			    return QVariant::fromValue(StarRating(value.toInt()));
		    }
		    value = QSqlQueryModel::data(index, Qt::DisplayRole);*/
		    switch (index.column()) {
                /*case PLAYLIST_TRACK:
	            {
                auto playing_index = QSqlQueryModel::index(index.row(), PLAYLIST_PLAYING);
                auto playing_index_value = QSqlQueryModel::data(playing_index, Qt::DisplayRole);
                if (!playing_index_value.toBool()) {
                    return QString::number(value.toInt()).rightJustified(2);
                }
                return {};
	            }
		    case PLAYLIST_YEAR:
                return QString::number(value.toInt()).rightJustified(8);
		    case PLAYLIST_FILE_SIZE:
                return QString::fromStdString(String::FormatBytes(value.toULongLong()));
		    case PLAYLIST_ALBUM_PK:
		    case PLAYLIST_ALBUM_RG:
            case PLAYLIST_TRACK_PK:
            case PLAYLIST_TRACK_RG:
                return QString::number(value.toFloat(), 'f', 2);
		    case PLAYLIST_BIT_RATE:
                return bitRate2String(value.toInt());
		    case PLAYLIST_SAMPLE_RATE:
			    return samplerate2String(value.toInt());
		    case PLAYLIST_DURATION:
			    return streamTimeToString(value.toDouble());
		    case PLAYLIST_LAST_UPDATE_TIME:
			    return QDateTime::fromSecsSinceEpoch(value.toULongLong()).toString(Q_TEXT("yyyy-MM-dd HH:mm:ss"));*/
		    }
	    }
        break;
    case Qt::DecorationRole:
        /*if (index.column() == PLAYLIST_TRACK) {
            auto playing_index = QSqlQueryModel::index(index.row(), PLAYLIST_PLAYING);
            auto value = QSqlQueryModel::data(playing_index, Qt::DisplayRole);
            auto playing_state = value.toInt();
            if (playing_state == PlayingState::PLAY_PLAYING) {
                return qTheme.iconFromFont(IconCode::ICON_Play);
            } else if (playing_state == PlayingState::PLAY_PAUSE) {
                return qTheme.iconFromFont(IconCode::ICON_Pause);
            }
            return {};
        }*/
        break;
    case Qt::TextAlignmentRole:
        /*switch (index.column()) {
        case PLAYLIST_FILE_SIZE:
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
        }*/
    default:
        break;
    }
    return QSqlQueryModel::data(index, role);
}
