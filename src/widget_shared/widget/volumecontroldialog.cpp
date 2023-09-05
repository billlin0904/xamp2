#include <widget/volumecontroldialog.h>

#include <thememanager.h>
#include <widget/str_utilts.h>
#include <widget/appsettingnames.h>
#include <widget/appsettings.h>
#include <widget/xmessagebox.h>

VolumeControlDialog::VolumeControlDialog(std::shared_ptr<IAudioPlayer> player, QWidget* parent)
	: QDialog(parent)
	, player_(player) {
	ui_.setupUi(this);
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);

    setAttribute(Qt::WA_StyledBackground);
	setFixedSize(35, 110);

	ui_.volumeSlider->SetRange(0, 100);
    ui_.volumeSlider->setFocusPolicy(Qt::NoFocus);
    ui_.volumeSlider->setInvertedAppearance(false);
    ui_.volumeSlider->setInvertedControls(false);

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
		SetVolume(volume);
        });

    (void)QObject::connect(ui_.volumeSlider, &QSlider::sliderMoved, [](auto volume) {
        });

    SetThemeColor();

    SetVolume(AppSettings::ValueAsInt(kAppSettingVolume));

    auto f = font();
    f.setFamily(qTEXT("MonoFont"));
    ui_.volumeLabel->setFont(f);
    ui_.volumeLabel->setStyleSheet(qTEXT("background-color: transparent; color: white;"));
    ui_.volumeSlider->setStyleSheet(qTEXT("background-color: transparent;"));

    qTheme.SetSliderTheme(ui_.volumeSlider, true);
}

void VolumeControlDialog::SetThemeColor() {
    switch (qTheme.GetThemeColor()) {
    case ThemeColor::DARK_THEME:
        setStyleSheet(qTEXT(R"(QDialog#VolumeControlDialog { background-color: black; border: none; })"));        
        break;
    case ThemeColor::LIGHT_THEME:
        setStyleSheet(qTEXT(R"(QDialog#VolumeControlDialog { background-color: gray; border: none; })"));
        break;
    }
    qTheme.SetSliderTheme(ui_.volumeSlider, true);
}

VolumeControlDialog::~VolumeControlDialog() {
    AppSettings::SetValue(kAppSettingVolume, ui_.volumeSlider->value());
}

void VolumeControlDialog::UpdateVolume() {
    ui_.volumeSlider->setValue(player_->GetVolume());
}

void VolumeControlDialog::SetVolume(uint32_t volume, bool notify) {
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
                ui_.volumeSlider->setDisabled(false);
            }
        }
        else {
            ui_.volumeSlider->setDisabled(true);
        }
        if (notify) {
            emit VolumeChanged(volume);
        }
        ui_.volumeLabel->setText(QString::number(volume));
    }
    catch (const Exception& e) {
        player_->Stop(false);
        XMessageBox::ShowError(qTEXT(e.GetErrorMessage()));
    }
}
