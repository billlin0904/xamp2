//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QToolButton>
#include <QTimer>

#include <widget/themecolor.h>
#include <widget/widget_shared_global.h>
#include <widget/widget_shared.h>

class VolumeControlDialog;

class XAMP_WIDGET_SHARED_EXPORT VolumeButton : public QToolButton {
	Q_OBJECT
public:
	explicit VolumeButton(QWidget *parent = nullptr);

    virtual ~VolumeButton();

	void SetPlayer(std::shared_ptr<IAudioPlayer> player);

public slots:
	void OnVolumeChanged(uint32_t volume);

	void OnCurrentThemeChanged(ThemeColor theme_color);

private:
	void mouseMoveEvent(QMouseEvent* event) override;
	
	void leaveEvent(QEvent* event) override;

	bool is_show_{ false };
	QTimer show_timer_;
	QScopedPointer<VolumeControlDialog> dialog_;
};
