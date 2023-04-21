//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QSqlQueryModel>

#include <widget/widget_shared_global.h>

class XAMP_WIDGET_SHARED_EXPORT PlayListSqlQueryTableModel : public QSqlQueryModel {
public:
    explicit PlayListSqlQueryTableModel(QObject *parent = nullptr);

    Qt::ItemFlags flags(const QModelIndex& index) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
};


