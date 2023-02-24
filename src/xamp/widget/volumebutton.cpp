#include "thememanager.h"
#include <widget/ui_utilts.h>
#include <widget/volumecontroldialog.h>
#include <widget/str_utilts.h>
#include <widget/volumebutton.h>

VolumeButton::VolumeButton(QWidget *parent)
	: QToolButton(parent) {
	setStyleSheet(qTEXT("background: transparent;"));
}

VolumeButton::~VolumeButton() = default;

void VolumeButton::SetPlayer(std::shared_ptr<IAudioPlayer> player) {
	dialog_.reset(new VolumeControlDialog(player, this));
	(void)QObject::connect(dialog_.get(),
		&VolumeControlDialog::VolumeChanged,
		this,
		&VolumeButton::OnVolumeChanged);
}

void VolumeButton::OnCurrentThemeChanged(ThemeColor theme_color) {
	dialog_->SetThemeColor();
}

void VolumeButton::OnVolumeChanged(uint32_t volume) {
	qTheme.SetMuted(this, volume == 0);
	dialog_->SetVolume(volume, false);
}

void VolumeButton::enterEvent(QEvent* event) {
	dialog_->SetThemeColor();
	dialog_->InitialVolumeControl();
	MoveToTopWidget(dialog_.get(), this);
	dialog_->show();
}

void VolumeButton::leaveEvent(QEvent* event) {
	dialog_->hide();
}
