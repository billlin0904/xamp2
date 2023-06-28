//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <ui_cdpage.h>
#include <thememanager.h>

#include <widget/widget_shared_global.h>

class PlaylistPage;

class XAMP_WIDGET_SHARED_EXPORT CdPage final : public QFrame {
    Q_OBJECT
public:
    explicit CdPage(QWidget* parent = nullptr);

    void showPlaylistPage(bool show);

    PlaylistPage* playlistPage() const {
        return ui_.playlistPage;
    }

public slots:
    void OnCurrentThemeChanged(ThemeColor theme_color);

private:
    Ui::CDPage ui_;
};