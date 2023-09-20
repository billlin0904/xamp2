#include <widget/equalizerview.h>

#include <widget/appsettingnames.h>
#include <widget/appsettings.h>
#include <widget/str_utilts.h>
#include <widget/widget_shared.h>
#include <stream/eqsettings.h>
#include <thememanager.h>

EqualizerView::EqualizerView(QWidget* parent)
    : QFrame(parent) {
    ui_.setupUi(this);

    freq_label_ = std::array<QLabel*, 10>{
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

    sliders_ = std::array<DoubleSlider*, 10>{
        ui_.band1Slider,
        ui_.band2Slider,
        ui_.band3Slider,
        ui_.band4Slider,
        ui_.band5Slider,
        ui_.band6Slider,
        ui_.band7Slider,
        ui_.band8Slider,
        ui_.band9Slider,
        ui_.band10Slider
    };

    bands_label_ = std::array<QLabel*, 10>{
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

    auto f = qTheme.GetMonoFont();
    f.setPointSize(qTheme.GetFontSize(8));

    ui_.preampLabel->setFont(f);

    for (auto& l : bands_label_) {
        l->setStyleSheet(qTEXT("background-color: transparent;"));
        l->setFont(f);
    }

    for (auto& l : freq_label_) {
        l->setStyleSheet(qTEXT("background-color: transparent;"));
        l->setFont(f);
    }

    qTheme.SetSliderTheme(ui_.preampSlider);

    auto band = 0;
    for (auto slider : sliders_) {
        qTheme.SetSliderTheme(slider);

        (void)QObject::connect(slider, &DoubleSlider::DoubleValueChanged, [band, this](auto value) {
            bands_label_[band]->setText(FormatDb(value));
            });
        ++band;
    }

    (void)QObject::connect(ui_.resetButton, &QPushButton::clicked, [this]() {
        auto [name, settings] = AppSettings::GetEqSettings();
        for (auto &band : settings.bands) {
            band.gain = 0;
        }
        settings.preamp = 0;
        ApplySetting(name, settings);
        });

    (void)QObject::connect(ui_.preampSlider, &DoubleSlider::DoubleValueChanged, [this](auto value) {
        PreampValueChange(value);
        ui_.preampDbLabel->setText(FormatDb(value));
        });

    ui_.enableEqCheckBox->setStyleSheet(qTEXT("background-color: transparent;"));
    (void)QObject::connect(ui_.enableEqCheckBox, &QCheckBox::stateChanged, [this](auto value) {
        AppSettings::SetValue(kAppSettingEnableEQ, value == Qt::CheckState::Checked);
        });

    (void)QObject::connect(ui_.enableEqCheckBox, &QCheckBox::stateChanged, [this](auto value) {
        AppSettings::SetValue(kAppSettingEnableEQ, value == Qt::CheckState::Checked);
        });

    ui_.enableEqCheckBox->setCheckState(AppSettings::ValueAsBool(kAppSettingEnableEQ)
        ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);

    for (auto& name : AppSettings::GetEqPreset().keys()) {
        ui_.eqPresetComboBox->addItem(name);
    }

    (void)QObject::connect(ui_.eqPresetComboBox, &QComboBox::textActivated, [this](auto index) {
        AppEQSettings settings;
        settings.name = index;
        settings.settings = AppSettings::GetEqPreset()[index];
        if (settings.settings.bands.empty()) {
            settings.settings.bands.push_back(EqBandSetting());
        }
        AppSettings::SetEqSettings(settings);
        AppSettings::save();
        ApplySetting(index, settings.settings);
        });

    if (AppSettings::contains(kAppSettingEQName)) {
        auto [name, settings] = AppSettings::GetEqSettings();
        AppEQSettings app_settings;
        app_settings.name = name;
        app_settings.settings = settings;
        AppSettings::SetEqSettings(app_settings);
        ui_.eqPresetComboBox->setCurrentText(name);
        ApplySetting(name, settings);
    }
}

void EqualizerView::ApplySetting(const QString& name, const EqSettings& settings) {
    for (auto& l : bands_label_) {
        l->hide();
    }
    for (auto& l : freq_label_) {
        l->hide();
    }
    for (auto& s : sliders_) {
        s->hide();
    }

    for (size_t i = 0; i < settings.bands.size(); ++i) {
        freq_label_[i]->setText(FormatSampleRate(settings.bands[i].frequency));
        bands_label_[i]->setText(FormatDb(settings.bands[i].gain, 2));
        sliders_[i]->setValue(settings.bands[i].gain * 10);

        freq_label_[i]->show();
        bands_label_[i]->show();
        sliders_[i]->show();
    }

    ui_.preampSlider->setValue(settings.preamp * 10);
    ui_.preampDbLabel->setText(FormatDb(settings.preamp));
}
