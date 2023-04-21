//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QDialog>

#include "ui_volumecontroldialog.h"
#include <widget/widget_shared.h>
#include <widget/xdialog.h>

class XAMP_WIDGET_SHARED_EXPORT VolumeControlDialog : public QDialog {
    Q_OBJECT
public:
	explicit VolumeControlDialog(std::shared_ptr<IAudioPlayer> player, QWidget* parent = nullptr);

	virtual ~VolumeControlDialog() override;

	void SetVolume(uint32_t volume, bool notify = true);

	void SetThemeColor();

signals:
    void VolumeChanged(uint32_t volume);

private:
	Ui::VolumeControlDialog ui_;
	std::shared_ptr<IAudioPlayer> player_;
};
