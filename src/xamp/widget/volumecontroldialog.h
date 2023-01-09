//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QDialog>

#include "ui_volumecontroldialog.h"
#include <widget/widget_shared.h>

namespace xamp {
	namespace player {
		class IAudioPlayer;
	}
}

class VolumeControlDialog : public QDialog {
public:
	explicit VolumeControlDialog(std::shared_ptr<IAudioPlayer> player, QWidget* parent = nullptr);

private:
	void setVolume(uint32_t volume);

	Ui::VolumeControlDialog ui_;
	std::shared_ptr<IAudioPlayer> player_;
};
