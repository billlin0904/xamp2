#include <QDir>
#include <QTextStream>
#include <QDirIterator>

#include <thememanager.h>

#include <widget/appsettings.h>
#include <widget/str_utilts.h>
#include <widget/equalizerdialog.h>

EqualizerDialog::EqualizerDialog(QWidget *parent)
    : QFrame(parent) {
    ui_.setupUi(this);

    Singleton<ThemeManager>::GetInstance().setBackgroundColor(this);

    band_sliders_ = std::vector<DoubleSlider*>{
            ui_.band1Slider,
            ui_.band2Slider,
            ui_.band3Slider,
            ui_.band4Slider,
            ui_.band5Slider,
            ui_.band6Slider,
            ui_.band7Slider,
            ui_.band8Slider,
            ui_.band9Slider,
            ui_.band10Slider,
            };

    band_label_ = std::vector<QLabel*>{
            ui_.band1DbLabel,
            ui_.band2DbLabel,
            ui_.band3DbLabel,
            ui_.band4DbLabel,
            ui_.band5DbLabel,
            ui_.band6DbLabel,
            ui_.band7DbLabel,
            ui_.band8DbLabel,
            ui_.band9DbLabel,
            ui_.band10DbLabel,
            };

    QFont f(Q_UTF8("MonoFont"));
    f.setPointSize(12);
    for (auto& l : band_label_) {
        l->setFont(f);
    }
    ui_.preampLabel->setFont(f);

    auto band = 0;
    for (auto& slider : band_sliders_) {
        (void)QObject::connect(slider, &DoubleSlider::doubleValueChanged, [band, this](auto value) {
            bandValueChange(band, value, 1.41);
            band_label_[band]->setText(QString(Q_UTF8("%1")).arg(value));

            auto settings = AppSettings::getValue(kAppSettingEQName).value<AppEQSettings>();
            settings.settings.bands[band].gain = value;
            settings.settings.bands[band].Q = 1.41;
            AppSettings::setValue(kAppSettingEQName, QVariant::fromValue(settings));
        });
        ++band;
    }

    (void)QObject::connect(ui_.preampSlider, &DoubleSlider::doubleValueChanged, [this](auto value) {
        preampValueChange(value);
        ui_.preampLabel->setText(QString(Q_UTF8("%1")).arg(value));
    });

    (void)QObject::connect(ui_.enableEqCheckBox, &QCheckBox::stateChanged, [this](auto value) {
        AppSettings::setValue(kAppSettingEnableEQ, value == Qt::CheckState::Checked);
    });

    ui_.enableEqCheckBox->setCheckState(AppSettings::getValueAsBool(kAppSettingEnableEQ) ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);

    for (auto &name : AppSettings::getEQPreset().keys()) {
        ui_.eqPresetComboBox->addItem(name);
    }

    (void)QObject::connect(ui_.eqPresetComboBox, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::activated), [this](auto index) {
        AppEQSettings settings;
        settings.name = index;
        settings.settings = AppSettings::getEQPreset()[index];
        AppSettings::setValue(kAppSettingEQName, QVariant::fromValue(settings));
        applySetting(index, settings.settings);
        AppSettings::save();
    });

    if (AppSettings::contains(kAppSettingEQName)) {
        auto [name, settings] = AppSettings::getValue(kAppSettingEQName).value<AppEQSettings>();
        ui_.eqPresetComboBox->setCurrentText(name);
        applySetting(name, settings);
    }

    (void)QObject::connect(ui_.resetButton, &QPushButton::pressed, [this]() {
        AppEQSettings settings;
        settings.name = ui_.eqPresetComboBox->currentText();
        for (auto& band : settings.settings.bands) {
	        band.Q = 1.41;
        }
        applySetting(settings.name, settings.settings);
        AppSettings::setValue(kAppSettingEQName, QVariant::fromValue(settings));
        AppSettings::save();
        });
}

void EqualizerDialog::applySetting(QString const& name, EQSettings const& settings) {
	if (!AppSettings::getEQPreset().contains(name)) {
        return;
    }

    for (auto i = 0; i < settings.bands.size(); ++i) {
        band_sliders_[i]->setValue(settings.bands[i].gain * 10);
        band_label_[i]->setText(QString(Q_UTF8("%1")).arg(settings.bands[i].gain));
    }
    ui_.preampSlider->setValue(settings.preamp * 10);
    ui_.preampLabel->setText(QString(Q_UTF8("%1")).arg(settings.preamp));
}
