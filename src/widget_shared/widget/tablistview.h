//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QMap>
#include <QListView>
#include <QStandardItemModel>

#include <widget/themecolor.h>
#include <widget/widget_shared_global.h>

enum TabIndex {
    TAB_PLAYLIST,
    TAB_FILE_EXPLORER,
    TAB_LYRICS,
    TAB_MUSIC_LIBRARY,
    TAB_CD,
};

class XAMP_WIDGET_SHARED_EXPORT TabListView : public QListView {
    Q_OBJECT
public:
    explicit TabListView(QWidget *parent = nullptr);

    void AddTab(const QString& name, int table_id, const QIcon &icon);

    void AddSeparator();

    QString GetTabName(int table_id) const;

    int32_t GetTabId(const QString &name) const;

signals:
    void ClickedTable(int table_id);

    void TableNameChanged(int table_id, const QString &name);

public slots:
    void OnCurrentThemeChanged(ThemeColor theme_color);

private:
    QStandardItemModel model_;
    QMap<int, QString> names_;
    QMap<QString, int> ids_;
};

