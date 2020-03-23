//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <unordered_set>
#include <QSortFilterProxyModel>

class PlayListTableFilterProxyModel : public QSortFilterProxyModel {
    Q_OBJECT
public:
    explicit PlayListTableFilterProxyModel(QObject *parent = nullptr);

    void setFilterByColumn(int32_t column);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;

private:
    std::unordered_set<int32_t> filters_;
};
