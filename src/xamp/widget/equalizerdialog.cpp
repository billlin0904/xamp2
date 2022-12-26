#include <QTextStream>
#include <widget/appsettingnames.h>
#include <widget/appsettings.h>
#include <widget/str_utilts.h>
#include <widget/equalizerdialog.h>

EqualizerDialog::EqualizerDialog(QWidget *parent)
    : QFrame(parent) {
    ui_.setupUi(this);

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
            ui_.preampDbLabel,
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

    band_feq_label_ = std::vector<QLabel*>{
            ui_.preampLabel,
            ui_.band1FeqLabel,
            ui_.band2FeqLabel,
            ui_.band3FeqLabel,
            ui_.band4FeqLabel,
            ui_.band5FeqLabel,
            ui_.band6FeqLabel,
            ui_.band7FeqLabel,
            ui_.band8FeqLabel,
            ui_.band9FeqLabel,
            ui_.band10FeqLabel,
            };

    QFont f(qTEXT("MonoFont"));
    f.setPointSize(8);
    for (auto& l : band_label_) {
        l->setFont(f);
        l->setStyleSheet(qTEXT("background-color: transparent;"));
    }
    for (auto& l : band_feq_label_) {
        l->setFont(f);
        l->setStyleSheet(qTEXT("background-color: transparent;"));
    }

    auto band = 0;
    for (auto& slider : band_sliders_) {
        (void)QObject::connect(slider, &DoubleSlider::doubleValueChanged, [band, this](auto value) {
            bandValueChange(band, value, 1.41);
            band_label_[band]->setText(QString(qTEXT("%1")).arg(value));

            auto settings = AppSettings::getEQSettings();
            settings.settings.bands[band].gain = value;
            settings.settings.bands[band].Q = 1.41;            
            AppSettings::setEQSettings(settings);
        });
        ++band;
    }

    (void)QObject::connect(ui_.preampSlider, &DoubleSlider::doubleValueChanged, [this](auto value) {
        preampValueChange(value);
        ui_.preampLabel->setText(QString(qTEXT("%1")).arg(value));
    });

    (void)QObject::connect(ui_.enableEqCheckBox, &QCheckBox::stateChanged, [this](auto value) {
        AppSettings::setValue(kAppSettingEnableEQ, value == Qt::CheckState::Checked);
    });

    ui_.enableEqCheckBox->setCheckState(AppSettings::getValueAsBool(kAppSettingEnableEQ) 
        ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);

    for (auto &name : AppSettings::getEQPreset().keys()) {
        ui_.eqPresetComboBox->addItem(name);
    }

    (void)QObject::connect(ui_.eqPresetComboBox, &QComboBox::textActivated, [this](auto index) {
        AppEQSettings settings;
        settings.name = index;
        settings.settings = AppSettings::getEQPreset()[index];
        AppSettings::setEQSettings(settings);
        applySetting(index, settings.settings);
        AppSettings::save();
    });

    if (AppSettings::contains(kAppSettingEQName)) {
        auto [name, settings] = AppSettings::getEQSettings();
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
        AppSettings::setEQSettings(settings);
        AppSettings::save();
        });
}

void EqualizerDialog::applySetting(QString const& name, EQSettings const& settings) {
	if (!AppSettings::getEQPreset().contains(name)) {
        return;
    }

    for (size_t i = 0; i < settings.bands.size(); ++i) {
        band_sliders_[i]->setValue(settings.bands[i].gain * 10);
        band_label_[i]->setText(QString(qTEXT("%1")).arg(settings.bands[i].gain));
    }
    ui_.preampSlider->setValue(settings.preamp * 10);
    ui_.preampLabel->setText(QString(qTEXT("%1")).arg(settings.preamp));
}
