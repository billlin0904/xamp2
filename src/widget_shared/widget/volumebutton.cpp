#include <widget/volumebutton.h>

#include <thememanager.h>
#include <QMouseEvent>

#include <base/logger_impl.h>

#include <widget/util/ui_util.h>
#include <widget/util/str_util.h>
#include <widget/volumecontroldialog.h>

constexpr auto kShowDelayMs = 100;
constexpr auto kAutoHideDelayMs = 2000;

VolumeButton::VolumeButton(QWidget *parent)
	: QToolButton(parent) {
	setStyleSheet("background: transparent;"_str);	
}

VolumeButton::~VolumeButton() = default;

void VolumeButton::setAudioPlayer(const std::shared_ptr<IAudioPlayer>& player) {
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
		hide_timer_.start(kAutoHideDelayMs);
		});
	(void)QObject::connect(&hide_timer_,
		&QTimer::timeout,
		[this]() {
		dialog_->hide();
		});
	setMouseTracking(true);
}

void VolumeButton::showDialog() {
	show_timer_.start(kShowDelayMs);
}

void VolumeButton::updateState() {
	dialog_->updateState();
}

void VolumeButton::onThemeChangedFinished(ThemeColor theme_color) {
	dialog_->setThemeColor();
}

void VolumeButton::onVolumeChanged(uint32_t volume) {
	dialog_->setVolume(volume, false);
	qTheme.setMuted(this, volume == 0);
	hide_timer_.stop();
	hide_timer_.start(kAutoHideDelayMs);
}

bool VolumeButton::eventFilter(QObject* obj, QEvent* e) {
	if (obj == this) {
		if (QEvent::WindowDeactivate == e->type()) {
			hide();
			hide_timer_.stop();
			return true;
		}
	}
	return QWidget::eventFilter(obj, e);
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
	show_timer_.stop();
}
