#include <QSqlDriver>

#include <widget/playlistsqlquerytablemodel.h>

#include <thememanager.h>
#include <widget/playlisttablemodel.h>
#include <widget/playlistentity.h>

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

QVariant PlayListSqlQueryTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal) {
        if (role == Qt::TextAlignmentRole) {
            if (section == PLAYLIST_ARTIST || section == PLAYLIST_DURATION) {
                return {Qt::AlignVCenter | Qt::AlignRight};
            } else if (section == PLAYLIST_TRACK) {
                return {Qt::AlignCenter };
            }
        }
    }
    return QSqlQueryModel::headerData(section, orientation, role);
}
