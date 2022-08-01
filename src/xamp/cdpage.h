//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <ui_cdpage.h>

class PlaylistPage;

class CdPage final : public QFrame {
    Q_OBJECT
public:
    explicit CdPage(QWidget* parent = nullptr);

    void showPlaylistPage(bool show);

    PlaylistPage* playlistPage() {
        return ui.playlistPage;
    }
private slots:
    
private:
    Ui::CDPage ui;
};