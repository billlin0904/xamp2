#include <QSqlDriver>

#include <widget/playlistsqlquerytablemodel.h>

#include <thememanager.h>
#include <widget/playlisttablemodel.h>
#include <widget/playlistentity.h>

PlayListSqlQueryTableModel::PlayListSqlQueryTableModel(QObject *parent)
    : QSqlQueryModel(parent) {
}

//QVariant PlayListSqlQueryTableModel::data(const QModelIndex& index, int role) const {
//    QVariant value = QSqlQueryModel::data(index, role);
//
//    if (role == Qt::CheckStateRole && index.column() == PLAYLIST_CHECKED) {
//        return (QSqlQueryModel::data(index).toInt() != 0) ? Qt::Checked : Qt::Unchecked;
//    }
//    return value;
//}
//
//bool PlayListSqlQueryTableModel::setData(const QModelIndex& index, const QVariant& value, int role) {
//    if (index.column() == PLAYLIST_CHECKED) {
//        role = Qt::CheckStateRole;
//    }
//    QSqlQueryModel::setData(index, value);
//    return true;
//}

Qt::ItemFlags PlayListSqlQueryTableModel::flags(const QModelIndex& index) const {
    if (!index.isValid()) {
        return QAbstractTableModel::flags(index);
    }
    if (index.column() == PLAYLIST_CHECKED) {
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEditable;
    }
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
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
