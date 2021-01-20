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
        return QAbstractItemModel::flags(index);
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
    case Qt::DisplayRole:
        if (index.column() == PLAYLIST_PLAYING) {
            return QVariant();
        }
        else if (index.column() == PLAYLIST_RATING) {
            value = QSqlQueryModel::data(index, Qt::DisplayRole);
            return QVariant::fromValue(StarRating(value.toInt()));
        } else {
            value = QSqlQueryModel::data(index, Qt::DisplayRole);
            switch (index.column()) {
            case PLAYLIST_BITRATE:
                if (value.toInt() > 10000) {
                    return QString(Q_UTF8("%0 Mbps")).arg(value.toInt() / 1000.0);
                }
                return QString(Q_UTF8("%0 Kbps")).arg(value.toInt());
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
