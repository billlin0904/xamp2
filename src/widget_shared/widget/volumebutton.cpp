#include <widget/volumebutton.h>

#include <thememanager.h>
#include <QMouseEvent>
#include <base/logger_impl.h>
#include <widget/util/ui_utilts.h>
#include <widget/volumecontroldialog.h>
#include <widget/util/str_utilts.h>

static constexpr auto kShowDelayMs = 100;

VolumeButton::VolumeButton(QWidget *parent)
	: QToolButton(parent) {
	setStyleSheet(qTEXT("background: transparent;"));
}

VolumeButton::~VolumeButton() = default;

void VolumeButton::setPlayer(std::shared_ptr<IAudioPlayer> player) {
	dialog_.reset(new VolumeControlDialog(player, this));
	(void)QObject::connect(dialog_.get(),
		&VolumeControlDialog::volumeChanged,
		this,
		&VolumeButton::onVolumeChanged);
	(void)QObject::connect(&show_timer_,
		&QTimer::timeout,
		[this]() {
		show_timer_.stop();
		is_show_ = true;

		dialog_->setThemeColor();
		dialog_->updateVolume();
		moveToTopWidget(dialog_.get(), this);
		dialog_->show();
		});
	setMouseTracking(true);
}

void VolumeButton::showDialog() {
	show_timer_.start(kShowDelayMs);
}

void VolumeButton::onThemeChangedFinished(ThemeColor theme_color) {
	dialog_->setThemeColor();
}

void VolumeButton::onVolumeChanged(uint32_t volume) {
	qTheme.setMuted(this, volume == 0);
	dialog_->setVolume(volume, false);
}

void VolumeButton::enterEvent(QEnterEvent* event) {
	if (is_show_) {
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
