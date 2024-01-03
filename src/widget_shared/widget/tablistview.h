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
    TAB_YT_MUSIC,
    TAB_YT_MUSIC_PLAYLIST,
};

class XAMP_WIDGET_SHARED_EXPORT TabListView final : public QListView {
    Q_OBJECT
public:
    explicit TabListView(QWidget *parent = nullptr);

    void addTab(const QString& name, int table_id, const QIcon &icon);

    void addSeparator();

    QString tabName(int table_id) const;

    int32_t tabId(const QString &name) const;

signals:
    void clickedTable(int table_id);

    void tableNameChanged(int table_id, const QString &name);

public slots:
    void onCurrentThemeChanged(ThemeColor theme_color);

private:
    QStandardItemModel model_;
    QMap<int, QString> names_;
    QMap<QString, int> ids_;
};

