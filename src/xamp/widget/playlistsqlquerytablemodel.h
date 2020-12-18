//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QSqlQueryModel>

class PlayListSqlQueryTableModel : public QSqlQueryModel {
public:
    explicit PlayListSqlQueryTableModel(QObject *parent = nullptr);

    Qt::ItemFlags flags(const QModelIndex& index) const override;

    QVariant data(const QModelIndex& index, int32_t role = Qt::DisplayRole) const override;
};


