//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QDialog>

#include <widget/widget_shared.h>
#include <widget/xdialog.h>

namespace Ui {
	class VolumeControlDialog;
}

class XAMP_WIDGET_SHARED_EXPORT VolumeControlDialog final : public QDialog {
    Q_OBJECT
public:
	explicit VolumeControlDialog(const std::shared_ptr<IAudioPlayer>& player, QWidget* parent = nullptr);

	virtual ~VolumeControlDialog() override;

	void setVolume(uint32_t volume, bool notify = true);

	void setThemeColor();

	void updateVolume();

	void paintEvent(QPaintEvent* event) override;

	bool isHardwareControlVolume() const;

	void updateState();
signals:
    void volumeChanged(uint32_t volume);

private:
	Ui::VolumeControlDialog* ui_;
	std::shared_ptr<IAudioPlayer> player_;
};
