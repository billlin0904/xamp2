#pragma once

#include <QSqlQueryModel>

class PlayListSqlQueryTableModel : public QSqlQueryModel {
public:
    explicit PlayListSqlQueryTableModel(QObject *parent = nullptr);

    QVariant data(const QModelIndex& index, int32_t role = Qt::DisplayRole) const override;
};


