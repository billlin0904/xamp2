#include <widget/volumebutton.h>

#include <thememanager.h>
#include <widget/appsettings.h>
#include <widget/appsettingnames.h>
#include <widget/util/str_util.h>

VolumeButton::VolumeButton(QWidget* parent)
    : QToolButton(parent) {
    setStyleSheet("background: transparent;"_str);
}

VolumeButton::~VolumeButton() = default;

void VolumeButton::setAudioPlayer(const std::shared_ptr<IAudioPlayer>& player) {
    player_ = player;
}

void VolumeButton::updateState() {
    if (!player_) return;
    if (player_->isHardwareControlVolume()) {
        volume_ = 100;
        qTheme.setMuted(this, false);
        emit volumeChanged(100);
        return;
    }
    onVolumeChanged(qAppSettings.valueAsBool(kAppSettingIsMuted)
        ? 0 : qAppSettings.valueAsInt(kAppSettingVolume));
}

void VolumeButton::onThemeChangedFinished(ThemeColor) {
    qTheme.setMuted(this, volume_ == 0);
}

void VolumeButton::onVolumeChanged(uint32_t volume) {
    if (!player_ || volume > 100 || player_->isHardwareControlVolume()) return;
    try {
        player_->setMute(volume == 0);
        if (volume > 0) player_->setVolume(volume);
        qAppSettings.setValue(kAppSettingVolume, volume);
        volume_ = volume;
        qTheme.setMuted(this, volume == 0);
        emit volumeChanged(static_cast<int>(volume));
    } catch (const std::exception&) {
        player_->stop(false);
    }
}
