#include <widget/equalizerview.h>

#include <widget/appsettingnames.h>
#include <widget/appsettings.h>
#include <widget/parametriceqview.h>
#include <widget/str_utilts.h>
#include <widget/widget_shared.h>
#include <stream/eqsettings.h>
#include <thememanager.h>

EqualizerView::EqualizerView(QWidget* parent)
    : QFrame(parent) {
    ui_.setupUi(this);

    QList<QLabel*> transparent_label{
        ui_.label_2,
        ui_.label_23,
        ui_.label_12,
        ui_.label_13,
        ui_.label_14,
        ui_.label_15,
    };

    for (auto& l : transparent_label) {
        l->setStyleSheet(qTEXT("background-color: transparent;"));
    }

    ui_.enableEqCheckBox->setStyleSheet(qTEXT("background-color: transparent;"));
    
    auto f = qTheme.GetMonoFont();
    f.setPointSize(qTheme.GetFontSize(8));

    ui_.preampLabel->setFont(f);
    ui_.preampDbLabel->setFont(f);

    ui_.preampLabel->setStyleSheet(qTEXT("background-color: transparent;"));
    ui_.preampDbLabel->setStyleSheet(qTEXT("background-color: transparent;"));

    qTheme.SetSliderTheme(ui_.preampSlider, true);

    (void)QObject::connect(ui_.preampSlider, &DoubleSlider::DoubleValueChanged, [this](auto value) {
        PreampValueChange(value);
        ui_.preampDbLabel->setText(FormatDb(value));
        });

    (void)QObject::connect(ui_.enableEqCheckBox, &QCheckBox::stateChanged, [this](auto value) {
        AppSettings::SetValue(kAppSettingEnableEQ, value == Qt::CheckState::Checked);
        });

    ui_.enableEqCheckBox->setCheckState(AppSettings::ValueAsBool(kAppSettingEnableEQ)
        ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);

    for (auto& name : AppSettings::GetEqPreset().keys()) {
        ui_.eqPresetComboBox->addItem(name);
    }

   for (auto i = 0; i < kEqDefaultFrequencies.size(); ++i) {
		filter_settings_.push_back(FilterSetting());
   		AddBand(i);
        RemoveBand(i);
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

    (void)QObject::connect(ui_.addBandButton, &QPushButton::pressed, [this]() {
        auto name = ui_.eqPresetComboBox->currentText();
        auto settings = AppSettings::GetEqPreset()[name];
        for (size_t i = settings.bands.size(); i < filter_settings_.size(); ++i) {
            filter_settings_[i].band->show();
            filter_settings_[i].frequency->show();
            filter_settings_[i].gain->show();
            filter_settings_[i].Q->show();
            filter_settings_[i].band_width->show();
            settings.bands.push_back(EqBandSetting());
            AppEQSettings app_settings{ name, settings };
            AppSettings::SetEqSettings(app_settings);
            AppSettings::save();
            break;
        }
        });
}

void EqualizerView::Save(uint32_t band) {
    AppEQSettings settings;
    settings.name = ui_.eqPresetComboBox->currentText();
    settings.settings = AppSettings::GetEqPreset()[settings.name];
    settings.settings.SetDefault();
    settings.settings.bands[band].gain = filter_settings_[band].gain->text().toFloat();
    settings.settings.bands[band].Q = filter_settings_[band].Q->text().toFloat();
    settings.settings.preamp = ui_.preampSlider->value() / 10.0;
    AppSettings::SetEqSettings(settings);
    AppSettings::save();
}

void EqualizerView::OnFftResultChanged(const ComplexValarray& result) {
    fft_result_ = result;
}

void EqualizerView::ApplySetting(QString const& name, EqSettings const& settings) {
    ui_.parametricEqView->ClearBand();
    ui_.parametricEqView->InitialAxisTicker(settings);

    for (size_t i = 0; i < settings.bands.size(); ++i) {
        SetBand(i, 
            settings.bands[i].type,
            settings.bands[i].frequency,
            settings.bands[i].gain,
            settings.bands[i].Q,
            settings.bands[i].band_width);
    }
    for (size_t i = settings.bands.size(); i < filter_settings_.size(); ++i) {
        RemoveBand(i);
    }
    ui_.preampSlider->setValue(settings.preamp * 10);
    ui_.preampDbLabel->setText(FormatDb(settings.preamp));
}

void EqualizerView::RemoveBand(uint32_t band) const {
    filter_settings_[band].band->hide();
    //filter_settings_[band].type->hide();
    filter_settings_[band].frequency->hide();
    filter_settings_[band].gain->hide();
    filter_settings_[band].Q->hide();
    filter_settings_[band].band_width->hide();
}

void EqualizerView::SetBandGain(uint32_t band, float gain) {
    filter_settings_[band].gain->setText(FormatDouble(gain, 2));
    ui_.parametricEqView->SetBand(kEqDefaultFrequencies[band], gain);
}

void EqualizerView::SetBand(uint32_t band, EQFilterTypes type, float frequency, float gain, float Q, float band_width) {
    filter_settings_[band].band->show();
    filter_settings_[band].frequency->show();
    filter_settings_[band].gain->show();
    filter_settings_[band].Q->show();
    filter_settings_[band].band_width->show();

    filter_settings_[band].frequency->setText(FormatDouble(frequency, 2));
    filter_settings_[band].gain->setText(FormatDouble(gain, 2));
    filter_settings_[band].Q->setText(FormatDouble(Q, 2));
    filter_settings_[band].band_width->setText(FormatDouble(frequency / 2, 2));

    ui_.parametricEqView->SetBand(frequency, gain);
}

void EqualizerView::SaveBand(int32_t index) {
    auto freq = filter_settings_[index].frequency->text().toFloat();
    auto gain = filter_settings_[index].gain->text().toFloat();
    auto q = filter_settings_[index].Q->text().toFloat();
    auto band_width = filter_settings_[index].band_width->text().toFloat();
    if (ui_.eqPresetComboBox->currentText() == tr("Manual")) {
        auto settings = AppSettings::GetEqPreset()[tr("Manual")];
        settings.bands[index].frequency = freq;
        settings.bands[index].Q = q;
        settings.bands[index].gain = gain;
        settings.bands[index].band_width = band_width;
        ApplySetting(tr("Manual"), settings);
        AppEQSettings app_settings;
        app_settings.name = tr("Manual");
        app_settings.settings = settings;
        AppSettings::SetEqSettings(app_settings);
        AppSettings::save();
    }
    else {
        SetBand(index, EQFilterTypes::FT_ALL_PASS, freq, gain, q, band_width);
    }
}

void EqualizerView::AddBand(uint32_t band) {
    auto * bandLabel = new QLabel(this);
    bandLabel->setObjectName(QString::fromUtf8("bandLabel"));
    bandLabel->setStyleSheet(qTEXT("background-color: transparent;"));
    bandLabel->setText(qSTR("Band %1").arg(band + 1));

    ui_.gridLayout->addWidget(bandLabel, band + 2, 1, 1, 1);
    QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);    

    auto* freqLineEdit = new QLineEdit(this);
    freqLineEdit->setObjectName(QString::fromUtf8("freqLineEdit"));
    sizePolicy.setHeightForWidth(freqLineEdit->sizePolicy().hasHeightForWidth());
    freqLineEdit->setSizePolicy(sizePolicy);
    freqLineEdit->setMaximumSize(QSize(110, 40));
    freqLineEdit->setAlignment(Qt::AlignRight);
    freqLineEdit->setReadOnly(true);

    ui_.gridLayout->addWidget(freqLineEdit, band + 2, 3, 1, 1);

    auto* gainLineEdit = new QLineEdit(this);
    gainLineEdit->setObjectName(QString::fromUtf8("gainLineEdit"));
    sizePolicy.setHeightForWidth(gainLineEdit->sizePolicy().hasHeightForWidth());
    gainLineEdit->setSizePolicy(sizePolicy);
    gainLineEdit->setMaximumSize(QSize(110, 40));
    auto* gain_validator = new QDoubleValidator(-12, 12, 2, this);
    gain_validator->setNotation(QDoubleValidator::StandardNotation);
    gainLineEdit->setValidator(gain_validator);
    gainLineEdit->setAlignment(Qt::AlignRight);
    gainLineEdit->setReadOnly(true);

    ui_.gridLayout->addWidget(gainLineEdit, band + 2, 5, 1, 1);

    auto* qLineEdit = new QLineEdit(this);
    qLineEdit->setObjectName(QString::fromUtf8("qLineEdit"));
    sizePolicy.setHeightForWidth(qLineEdit->sizePolicy().hasHeightForWidth());
    qLineEdit->setSizePolicy(sizePolicy);
    qLineEdit->setMaximumSize(QSize(110, 40));
    qLineEdit->setAlignment(Qt::AlignRight);
    qLineEdit->setReadOnly(true);

    ui_.gridLayout->addWidget(qLineEdit, band + 2, 6, 1, 1);

    auto* bandWidthLineEdit = new QLineEdit(this);
    bandWidthLineEdit->setObjectName(QString::fromUtf8("bandWidthLineEdit"));
    sizePolicy.setHeightForWidth(bandWidthLineEdit->sizePolicy().hasHeightForWidth());
    bandWidthLineEdit->setSizePolicy(sizePolicy);
    bandWidthLineEdit->setMaximumSize(QSize(110, 40));
    bandWidthLineEdit->setReadOnly(true);
    bandWidthLineEdit->setAlignment(Qt::AlignRight);

    ui_.gridLayout->addWidget(bandWidthLineEdit, band + 2, 8, 1, 1);   

    filter_settings_[band].band = bandLabel;
    filter_settings_[band].band_width = bandWidthLineEdit;
    filter_settings_[band].gain = gainLineEdit;
    filter_settings_[band].Q = qLineEdit;
    filter_settings_[band].frequency = freqLineEdit;

    auto index = band;
    (void)QObject::connect(bandWidthLineEdit, &QLineEdit::editingFinished, [index, this]() {
        SaveBand(index);
        });
    (void) QObject::connect(gainLineEdit, &QLineEdit::editingFinished, [index, this]() {
        SaveBand(index);
        });
    (void)QObject::connect(freqLineEdit, &QLineEdit::editingFinished, [index, this]() {
        SaveBand(index);
        });
    (void)QObject::connect(qLineEdit, &QLineEdit::editingFinished, [index, this]() {
        SaveBand(index);
        });
    (void)QObject::connect(freqLineEdit, &QLineEdit::editingFinished, [index, this]() {
        SaveBand(index);
        });

    const auto f = qTheme.GetMonoFont();
    filter_settings_[band].frequency->setFont(f);
    filter_settings_[band].gain->setFont(f);
    filter_settings_[band].Q->setFont(f);
    filter_settings_[band].band_width->setFont(f);
}
