//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QSqlQueryModel>

#include <widget/widget_shared_global.h>

class PlayListSqlQueryTableModel : public QSqlQueryModel {
public:
    explicit PlayListSqlQueryTableModel(QObject *parent = nullptr);

    Qt::ItemFlags flags(const QModelIndex& index) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

public slots:
    void setFilterColumn(const QString& column);

    void setFilterFlags(const Qt::MatchFlag flags);

    void setFilter(const QString& filter);

    void filter(const QString& filter);

    void select();

private:
    virtual void setSort(int column, Qt::SortOrder order);

    void sort(int column, Qt::SortOrder order) override;
};


