//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QSortFilterProxyModel>

#include <widget/widget_shared.h>
#include <widget/widget_shared_global.h>

class PlayListTableFilterProxyModel final : public QSortFilterProxyModel {
    Q_OBJECT
public:
    explicit PlayListTableFilterProxyModel(QObject *parent = nullptr);

    void AddFilterByColumn(int32_t column);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;

private:
    HashSet<int32_t> filters_;
};
