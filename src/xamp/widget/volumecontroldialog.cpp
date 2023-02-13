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
	ui_.volumeSlider->SetRange(0, 100);

    if (AppSettings::ValueAsBool(kAppSettingIsMuted)) {
        SetVolume(0);
    }
    else {
        const auto vol = AppSettings::GetValue(kAppSettingVolume).toUInt();
        SetVolume(vol);
        ui_.volumeSlider->setValue(vol);
    }

    (void)QObject::connect(ui_.volumeSlider, &QSlider::valueChanged, [this](auto volume) {
        SetVolume(volume);
        });

    (void)QObject::connect(ui_.volumeSlider, &SeekSlider::LeftButtonValueChanged, [this](auto volume) {
        QToolTip::showText(QCursor::pos(), tr("Volume : ") + QString::number(volume) + qTEXT("%"));
		SetVolume(volume);
        });

    (void)QObject::connect(ui_.volumeSlider, &QSlider::sliderMoved, [](auto volume) {
        QToolTip::showText(QCursor::pos(), tr("Volume : ") + QString::number(volume) + qTEXT("%"));
        });

    switch (qTheme.GetThemeColor()) {
    case ThemeColor::DARK_THEME:
        setStyleSheet(qTEXT(R"(QDialog#VolumeControlDialog { background-color: black; border: none; })"));
        break;
    case ThemeColor::LIGHT_THEME:
        setStyleSheet(qTEXT(R"(QDialog#VolumeControlDialog { background-color: gray; border: none; })"));
        break;
    }
    SetVolume(AppSettings::ValueAsInt(kAppSettingVolume));
    auto f = font();
    f.setFamily(qTEXT("MonoFont"));
    ui_.volumeLabel->setFont(f);
    ui_.volumeLabel->setStyleSheet(qTEXT("background-color: transparent; color: white;"));
    ui_.volumeSlider->setStyleSheet(qTEXT("background-color: transparent;"));

    qTheme.SetSliderTheme(ui_.volumeSlider, true);
}

VolumeControlDialog::~VolumeControlDialog() {
    AppSettings::SetValue(kAppSettingVolume, ui_.volumeSlider->value());
}

void VolumeControlDialog::SetVolume(uint32_t volume) {
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
            }
        }
        else {
            ui_.volumeSlider->setDisabled(true);
        }
        ui_.volumeLabel->setText(QString::number(volume));
    }
    catch (const Exception& e) {
        player_->Stop(false);
        XMessageBox::ShowError(qTEXT(e.GetErrorMessage()));
    }
}
