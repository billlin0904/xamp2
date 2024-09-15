//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>
#include <widget/widget_shared.h>

#include <widget/themecolor.h>
#include <widget/youtubedl/ytmusicservice.h>

struct PlayListEntity;
class Xamp;
class MusicPlayerModel;
class MbDiscIdInfo;
class PlaylistPage;
class DatabaseCoverId;

class MusicController : public QObject {
	Q_OBJECT
public:
    MusicController(MusicPlayerModel* model, Xamp* view, QObject* parent = nullptr);

public slots:

private:
	MusicPlayerModel* model_;
	Xamp* view_;
	std::shared_ptr<IAudioPlayer> player_;
};
