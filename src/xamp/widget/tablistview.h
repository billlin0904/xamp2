//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QMap>
#include <QListView>
#include <QStandardItemModel>

class TabListView : public QListView {
    Q_OBJECT
public:
    explicit TabListView(QWidget *parent = nullptr);

    void addTab(const QString& name, int table_id, const QIcon &icon);

    void addSeparator();

    QString getTabName(int table_id) const;

    int32_t getTabId(const QString &name) const;

signals:
    void clickedTable(int table_id);

    void tableNameChanged(int table_id, const QString &name);

private:
    QStandardItemModel model_;
    QMap<int, QString> names_;
    QMap<QString, int> ids_;
};

