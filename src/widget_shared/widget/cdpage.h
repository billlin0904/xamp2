//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <thememanager.h>
#include <widget/widget_shared_global.h>

class PlaylistPage;

namespace Ui {
    class CDPage;
}

class XAMP_WIDGET_SHARED_EXPORT CdPage final : public QFrame {
    Q_OBJECT
public:
    explicit CdPage(QWidget* parent = nullptr);

    virtual ~CdPage() override;

    void showPlaylistPage(bool show);

    PlaylistPage* playlistPage() const;

public slots:
    void onThemeChangedFinished(ThemeColor theme_color);

private:
    Ui::CDPage* ui_;
};