#include <QTextStream>
#include <widget/appsettingnames.h>
#include <widget/appsettings.h>
#include <widget/str_utilts.h>
#include <thememanager.h>

#include <widget/equalizerdialog.h>

EqualizerDialog::EqualizerDialog(QWidget *parent)
    : QFrame(parent) {
    ui_.setupUi(this);

    ui_.label_23->setStyleSheet(qTEXT("background-color: transparent;"));
    ui_.enableEqCheckBox->setStyleSheet(qTEXT("background-color: transparent;"));

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

    band_feq_label_ = std::vector<QLabel*>{
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
    f.setPointSize(qTheme.GetFontSize(8));
    for (auto& l : band_label_) {
        l->setFont(f);
        l->setStyleSheet(qTEXT("background-color: transparent;"));
    }
    ui_.preampDbLabel->setFont(f);
    for (auto& l : band_feq_label_) {
        l->setFont(f);
        l->setStyleSheet(qTEXT("background-color: transparent;"));
    }
    ui_.preampLabel->setFont(f);
    ui_.preampLabel->setStyleSheet(qTEXT("background-color: transparent;"));

    auto i = 0;
    for (auto& slider : band_sliders_) {
        (void)QObject::connect(slider, &DoubleSlider::DoubleValueChanged, [i, this](auto value) {
            BandValueChange(i, value, 1.41);
            band_label_[i]->setText(QString(qTEXT("%1")).arg(value));
            auto settings = AppSettings::GetEqSettings();
            settings.settings.bands[i].gain = value;
            settings.settings.bands[i].Q = 1.41;
            AppSettings::SetEqSettings(settings);
        });
        qTheme.SetSliderTheme(slider, true);
        ++i;
    }

    qTheme.SetSliderTheme(ui_.preampSlider, true);

    (void)QObject::connect(ui_.preampSlider, &DoubleSlider::DoubleValueChanged, [this](auto value) {
        PreampValueChange(value);
        ui_.preampLabel->setText(QString(qTEXT("%1")).arg(value));
    });

    (void)QObject::connect(ui_.enableEqCheckBox, &QCheckBox::stateChanged, [this](auto value) {
        AppSettings::SetValue(kAppSettingEnableEQ, value == Qt::CheckState::Checked);
    });

    ui_.enableEqCheckBox->setCheckState(AppSettings::ValueAsBool(kAppSettingEnableEQ) 
        ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);

    for (auto &name : AppSettings::GetEqPreset().keys()) {
        ui_.eqPresetComboBox->addItem(name);
    }

    (void)QObject::connect(ui_.eqPresetComboBox, &QComboBox::textActivated, [this](auto index) {
        AppEQSettings settings;
        settings.name = index;
        settings.settings = AppSettings::GetEqPreset()[index];
        AppSettings::SetEqSettings(settings);
        ApplySetting(index, settings.settings);
        AppSettings::save();
    });

    if (AppSettings::contains(kAppSettingEQName)) {
        auto [name, settings] = AppSettings::GetEqSettings();
        ui_.eqPresetComboBox->setCurrentText(name);
        ApplySetting(name, settings);
    }

    (void)QObject::connect(ui_.saveButton, &QPushButton::pressed, [this]() {
        AppEQSettings settings;
        settings.name = ui_.eqPresetComboBox->currentText();
        auto i = 0;
        for (const auto& slider : band_sliders_) {
            settings.settings.bands[i].gain = slider->value() / 10.0;
            settings.settings.bands[i].Q = 1.41;
            ++i;
        }
        settings.settings.preamp = ui_.preampSlider->value() / 10.0;
        ApplySetting(settings.name, settings.settings);
        AppSettings::SetEqSettings(settings);
        AppSettings::save();
        });

    (void)QObject::connect(ui_.resetButton, &QPushButton::pressed, [this]() {
        AppEQSettings settings;
        settings.name = ui_.eqPresetComboBox->currentText();
        for (auto& band : settings.settings.bands) {
	        band.Q = 1.41;
        }
        ApplySetting(settings.name, settings.settings);
        AppSettings::SetEqSettings(settings);
        AppSettings::save();
        });
}

void EqualizerDialog::ApplySetting(QString const& name, EQSettings const& settings) {
	if (!AppSettings::GetEqPreset().contains(name)) {
        return;
    }

    for (size_t i = 0; i < settings.bands.size(); ++i) {
        band_sliders_[i]->setValue(settings.bands[i].gain * 10);
        band_label_[i]->setText(QString(qTEXT("%1")).arg(settings.bands[i].gain));
    }
    ui_.preampSlider->setValue(settings.preamp * 10);
    ui_.preampLabel->setText(QString(qTEXT("%1")).arg(settings.preamp));
}
