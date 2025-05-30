//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QSqlQueryModel>

class PlayListSqlQueryTableModel final : public QSqlQueryModel {
public:
    explicit PlayListSqlQueryTableModel(QObject *parent = nullptr);

    Qt::ItemFlags flags(const QModelIndex& index) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
};


