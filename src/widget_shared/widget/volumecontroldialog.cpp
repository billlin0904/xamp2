#include <QPainter>
#include <widget/volumecontroldialog.h>
#include <ui_volumecontroldialog.h>
#include <thememanager.h>
#include <widget/util/str_utilts.h>
#include <widget/appsettingnames.h>
#include <widget/appsettings.h>

VolumeControlDialog::VolumeControlDialog(const std::shared_ptr<IAudioPlayer> &player, QWidget* parent)
	: QDialog(parent)
	, player_(player) {
    ui_ = new Ui::VolumeControlDialog();
	ui_->setupUi(this);
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);

    setAttribute(Qt::WA_TranslucentBackground);
	setFixedSize(35, 110);

	ui_->volumeSlider->setRange(0, 100);
    ui_->volumeSlider->setFocusPolicy(Qt::NoFocus);
    ui_->volumeSlider->setSingleStep(1);
    ui_->volumeSlider->setInvertedAppearance(false);
    ui_->volumeSlider->setInvertedControls(false);

    (void)QObject::connect(ui_->volumeSlider, &QSlider::valueChanged, [this](auto volume) {
        setVolume(volume);
        });

    (void)QObject::connect(ui_->volumeSlider, &SeekSlider::leftButtonValueChanged, [this](auto volume) {
		setVolume(volume);
        });

    (void)QObject::connect(ui_->volumeSlider, &QSlider::sliderMoved, [](auto volume) {
        });

    setThemeColor();

    setVolume(qAppSettings.valueAsInt(kAppSettingVolume));

    auto f = font();
    f.setFamily(qTEXT("MonoFont"));
    ui_->volumeLabel->setFont(f);
    ui_->volumeLabel->setStyleSheet(qTEXT("background-color: transparent; color: gray;"));
    ui_->volumeSlider->setStyleSheet(qTEXT("background-color: transparent;"));

    qTheme.setSliderTheme(ui_->volumeSlider, true);
}

void VolumeControlDialog::setThemeColor() {
    qTheme.setSliderTheme(ui_->volumeSlider, true);
    update();
}

VolumeControlDialog::~VolumeControlDialog() {
    qAppSettings.setValue(kAppSettingVolume, ui_->volumeSlider->value());
    delete ui_;
}

void VolumeControlDialog::updateVolume() {
    ui_->volumeSlider->setValue(player_->GetVolume());
}

void VolumeControlDialog::paintEvent(QPaintEvent* event) {
    QPainter p(this);
    p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    switch (qTheme.themeColor()) {
    case ThemeColor::DARK_THEME:
        p.setPen(QColor(15, 15, 15));
        p.setBrush(QColor(31, 31, 31));
        break;
    case ThemeColor::LIGHT_THEME:
        p.setPen(QColor(190, 190, 190));
        p.setBrush(QColor(235, 235, 235));
        break;
    }
    p.drawRoundedRect(rect(), 10, 10);
}

bool VolumeControlDialog::isHardwareControlVolume() const {
    return player_->IsHardwareControlVolume();
}

void VolumeControlDialog::updateState() {
    if (!player_->IsHardwareControlVolume()) {
        if (qAppSettings.valueAsBool(kAppSettingIsMuted)) {
            setVolume(0);
        }
        else {
            const auto vol = qAppSettings.valueAs(kAppSettingVolume).toUInt();
            setVolume(vol);
            ui_->volumeSlider->setValue(vol);
        }
    }
    else {
        ui_->volumeSlider->setValue(100);
        ui_->volumeSlider->setDisabled(true);
        ui_->volumeLabel->setText(QString::number(100));
        ui_->volumeLabel->adjustSize();
    }
}

void VolumeControlDialog::setVolume(uint32_t volume, bool notify) {
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

        if (!player_->IsHardwareControlVolume()) {            
            if (!player_->IsMute()) {
                player_->SetVolume(volume);
            }
            ui_->volumeSlider->setDisabled(false);
        }
        else {
            ui_->volumeSlider->setDisabled(true);
            return;
        }
        if (notify) {
            emit volumeChanged(volume);
        }
        ui_->volumeLabel->setText(QString::number(volume));
        ui_->volumeLabel->adjustSize();
    }
    catch (const Exception& e) {
        player_->Stop(false);
    }
}
