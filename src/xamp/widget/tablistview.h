//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QListView>
#include <QStandardItemModel>

class TabListView : public QListView {
    Q_OBJECT
public:
    explicit TabListView(QWidget *parent = nullptr);

    void addTab(const QString& name, int table_id, const QIcon &icon);

    void addSeparator();

signals:
    void clickedTable(int table_id);

    void tableNameChanged(int table_id, const QString &name);

private:
    QStandardItemModel model_;
};

