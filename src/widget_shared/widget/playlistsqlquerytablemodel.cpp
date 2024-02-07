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
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
}

QVariant PlayListSqlQueryTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal) {
        if (role == Qt::TextAlignmentRole) {
            if (section == PLAYLIST_TRACK) {
                return { Qt::AlignCenter };
            }
        }
    }
    return QSqlQueryModel::headerData(section, orientation, role);
}
