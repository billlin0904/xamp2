#include <widget/volumebutton.h>

#include <thememanager.h>

#include <base/logger_impl.h>
#include <widget/ui_utilts.h>
#include <widget/volumecontroldialog.h>
#include <widget/str_utilts.h>

static constexpr auto kShowDelayMs = 100;

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
	(void)QObject::connect(&show_timer_,
		&QTimer::timeout,
		[this]() {
		show_timer_.stop();
		is_show_ = true;

		dialog_->SetThemeColor();
		dialog_->UpdateVolume();
		MoveToTopWidget(dialog_.get(), this);
		dialog_->show();
		});
	setMouseTracking(true);
}

void VolumeButton::ShowDialog() {
	show_timer_.start(kShowDelayMs);
}

void VolumeButton::OnCurrentThemeChanged(ThemeColor theme_color) {
	dialog_->SetThemeColor();
}

void VolumeButton::OnVolumeChanged(uint32_t volume) {
	qTheme.SetMuted(this, volume == 0);
	dialog_->SetVolume(volume, false);
}

void VolumeButton::mouseMoveEvent(QMouseEvent* event) {
	if (is_show_) {
		return;
	}

	if (!rect().contains(event->pos())) {
		return;
	}

	if (!show_timer_.isActive()) {
		show_timer_.setInterval(kShowDelayMs);
		show_timer_.start();
	}
}

void VolumeButton::leaveEvent(QEvent* event) {
	if (!is_show_) {
		return;
	}
	is_show_ = false;
}
