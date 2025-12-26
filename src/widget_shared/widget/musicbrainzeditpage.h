//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <thememanager.h>
#include <widget/widget_shared_global.h>
#include <widget/worker/backgroundservice.h>
#include <widget/playlistentity.h>

namespace Ui {
    class MusicbrainzEditPage;
}

class XAMP_WIDGET_SHARED_EXPORT MusicbrainzEditPage final : public QFrame {
    Q_OBJECT
public:
    MusicbrainzEditPage(const QList<PlayListEntity>& entities, const QList<MusicBrainzAlbum>& recording_list, QWidget* parent = nullptr);

    virtual ~MusicbrainzEditPage() override;

public slots:
    void onThemeChangedFinished(ThemeColor theme_color);

    void onRetranslateUi();

private:
    void load(const QList<PlayListEntity>& entities, const QList<MusicBrainzAlbum>& recording_list);
    QStandardItemModel* track_model_;
    QStandardItemModel* album_track_model_;
    QStandardItemModel* tag_model_;
    QMap<QString, QPixmap> cover_art_map_;
    Ui::MusicbrainzEditPage* ui_;
};
