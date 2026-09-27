//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QToolButton>

#include <widget/themecolor.h>
#include <widget/widget_shared_global.h>
#include <widget/widget_shared.h>
#include <player/iaudioplayer.h>


class XAMP_WIDGET_SHARED_API VolumeButton : public QToolButton {
	Q_OBJECT
public:
	explicit VolumeButton(QWidget *parent = nullptr);

	~VolumeButton() override;

	void setAudioPlayer(const std::shared_ptr<IAudioPlayer>& player);


	void updateState();
signals:
    void volumeChanged(int volume);

public slots:
	void onVolumeChanged(uint32_t volume);

	void onThemeChangedFinished(ThemeColor theme_color);

private:
    std::shared_ptr<IAudioPlayer> player_;
    uint32_t volume_{0};
};
