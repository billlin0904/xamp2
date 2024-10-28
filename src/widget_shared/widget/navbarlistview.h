//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QMap>
#include <QListView>
#include <QStandardItemModel>
#include <QElapsedTimer>

#include <widget/themecolor.h>
#include <widget/widget_shared_global.h>

enum TabIndex {
    TAB_PLAYLIST,
    TAB_FILE_EXPLORER,
    TAB_LYRICS,
    TAB_MUSIC_LIBRARY,
    TAB_CD,
    TAB_YT_MUSIC_SEARCH,
    TAB_YT_MUSIC_PLAYLIST,
    TAB_AI,
};

class XTooltip;

class XAMP_WIDGET_SHARED_EXPORT NavBarListView final : public QListView {
    Q_OBJECT
public:
    explicit NavBarListView(QWidget *parent = nullptr);

    void addTab(const QString& name, int table_id, const QIcon& icon);

    void addSeparator();

    QString tabName(int table_id) const;

    int32_t tabId(const QString &name) const;

	int32_t currentTabId() const;

    void setTabText(const QString& name, int table_id);

    void mouseMoveEvent(QMouseEvent* event) override;

    void toolTipMove(const QPoint &pos);

signals:
    void clickedTable(int table_id);

    void tableNameChanged(int table_id, const QString &name);

public slots:
    void onThemeChangedFinished(ThemeColor theme_color);
    
    void onRetranslateUi();

private:
    QStandardItemModel model_;
    QMap<int, QString> names_;
    QMap<QString, int> ids_;
    XTooltip* tooltip_;
	QElapsedTimer elapsed_timer_;
};

