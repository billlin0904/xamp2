//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
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

	~VolumeButton() override;

	void setPlayer(std::shared_ptr<IAudioPlayer> player);

	void showDialog();

public slots:
	void onVolumeChanged(uint32_t volume);

	void onCurrentThemeChanged(ThemeColor theme_color);

private:
	void mouseMoveEvent(QMouseEvent* event) override;
	
	void leaveEvent(QEvent* event) override;

	bool is_show_{ false };
	QTimer show_timer_;
	QScopedPointer<VolumeControlDialog> dialog_;
};
