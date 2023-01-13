#include <widget/volumecontroldialog.h>


#include "thememanager.h"
#include <widget/str_utilts.h>
#include <widget/appsettingnames.h>
#include <widget/appsettings.h>
#include <widget/xmessagebox.h>

#include <QToolTip>

VolumeControlDialog::VolumeControlDialog(std::shared_ptr<IAudioPlayer> player, QWidget* parent)
	: QDialog(parent)
	, player_(player) {
	ui_.setupUi(this);
	setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_StyledBackground);
	setFixedSize(30, 110);

    ui_.volumeButton->setIconSize(QSize(20, 20));
    ui_.volumeButton->setFixedSize(20, 20);
	ui_.volumeSlider->setRange(0, 100);
    ui_.volumeButton->setStyleSheet(qTEXT(R"(
                                         QPushButton#volumeButton {
                                         border: none;
                                         background-color: transparent;
                                         }
                                         )"));

    if (AppSettings::getValueAsBool(kAppSettingIsMuted)) {
        setVolume(0);
    }
    else {
        const auto vol = AppSettings::getValue(kAppSettingVolume).toUInt();
        setVolume(vol);
        ui_.volumeSlider->setValue(vol);
    }

    (void)QObject::connect(ui_.volumeSlider, &QSlider::valueChanged, [this](auto volume) {
        setVolume(volume);
        });

    (void)QObject::connect(ui_.volumeSlider, &SeekSlider::leftButtonValueChanged, [this](auto volume) {
        QToolTip::showText(QCursor::pos(), tr("Volume : ") + QString::number(volume) + qTEXT("%"));
		setVolume(volume);
        });

    (void)QObject::connect(ui_.volumeSlider, &QSlider::sliderMoved, [](auto volume) {
        QToolTip::showText(QCursor::pos(), tr("Volume : ") + QString::number(volume) + qTEXT("%"));
        });

    (void)QObject::connect(ui_.volumeButton, &QPushButton::pressed, [this]() {
        if (!ui_.volumeSlider->isEnabled()) {
            return;
        }
        if (player_->IsMute()) {
            player_->SetMute(false);
            qTheme.setMuted(ui_.volumeButton, false);
        } else {
            player_->SetMute(true);
            qTheme.setMuted(ui_.volumeButton, true);
        }
        });

    switch (qTheme.themeColor()) {
    case ThemeColor::DARK_THEME:
        setStyleSheet(qTEXT(R"(QDialog#VolumeControlDialog { background-color: #121212; border: 1px solid #a7c8ff; })"));
        break;
    default:
        setStyleSheet(qTEXT(R"(QDialog#VolumeControlDialog { border: 1px solid gray; })"));
        break;
    }
}

VolumeControlDialog::~VolumeControlDialog() {
    AppSettings::setValue(kAppSettingVolume, ui_.volumeSlider->value());
}

void VolumeControlDialog::setVolume(uint32_t volume) {
    if (volume > 100) {
        return;
    }

    try {
        if (volume > 0) {
            player_->SetMute(false);
        }
        else {
            player_->SetMute(true);
        }

        if (player_->IsHardwareControlVolume()) {
            if (!player_->IsMute()) {
                player_->SetVolume(volume);
                qTheme.setVolume(ui_.volumeSlider, ui_.volumeButton, volume);
            }
            else {
                qTheme.setVolume(ui_.volumeSlider, ui_.volumeButton, 0);
            }
        }
        else {
            ui_.volumeSlider->setDisabled(true);
        }
    }
    catch (const Exception& e) {
        player_->Stop(false);
        XMessageBox::showError(qTEXT(e.GetErrorMessage()));
    }
}