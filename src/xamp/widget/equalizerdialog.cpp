#include <QTextStream>
#include <widget/appsettingnames.h>
#include <widget/appsettings.h>
#include <widget/parametriceqview.h>
#include <widget/str_utilts.h>
#include <widget/widget_shared.h>
#include <stream/eqsettings.h>
#include <thememanager.h>

#include <widget/equalizerdialog.h>

static uint32_t Index2Freq(int32_t i, uint32_t samples_rate, uint32_t fft_data_size) {
    return static_cast<uint32_t>(i) * (samples_rate / fft_data_size / 2.);
}

static uint32_t Freq2Index(uint32_t freq, uint32_t samples_rate, uint32_t fft_data_size) {
    return static_cast<uint32_t>(freq / (samples_rate / fft_data_size / 2.0));
}

static float ToMag(const std::complex<float>& r) {
    if (r.real() == 0 && r.imag() == 0) {
        return 0;
    }
    return 10.0 * std::log10(std::pow(r.real(), 2) + std::pow(r.imag(), 2));
}

static float ToDb(const std::complex<float>& r) {
    if (r.real() == 0 && r.imag() == 0) {
        return 0;
    }
    auto mag = std::sqrt(std::pow(r.real(), 2) + std::pow(r.imag(), 2));
    return 20.0 * std::log10(mag);
}

EqualizerDialog::EqualizerDialog(QWidget* parent)
    : QFrame(parent) {
    ui_.setupUi(this);

    QList<QLabel*> transparent_label{
        ui_.label_2,
        ui_.label_23,
        ui_.label_11,
        ui_.label_12,
        ui_.label_13,
        ui_.label_14,
        ui_.label_15,
    };

    for (auto& l : transparent_label) {
        l->setStyleSheet(qTEXT("background-color: transparent;"));
    }

    ui_.enableEqCheckBox->setStyleSheet(qTEXT("background-color: transparent;"));

    band_sliders_ = QList<DoubleSlider*>{
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

    band_label_ = QList<QLabel*>{
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

    band_feq_label_ = QList<QLabel*>{
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

    auto f = qTheme.MonoFont();

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
    ui_.preampDbLabel->setStyleSheet(qTEXT("background-color: transparent;"));

    parametric_eq_view_ = new ParametricEqView(this);
    ui_.verticalLayout_4->insertWidget(1, parametric_eq_view_);

    auto i = 0;
    for (auto& slider : band_sliders_) {
        (void)QObject::connect(slider, &DoubleSlider::DoubleValueChanged, [i, this](auto value) {
            BandValueChange(i, value, kDefaultQ);
            band_label_[i]->setText(FormatDb(value));
            auto settings = AppSettings::GetEqSettings();
            settings.settings.bands[i].gain = value;
            settings.settings.bands[i].Q = kDefaultQ;
            AppSettings::SetEqSettings(settings);
            });
        qTheme.SetSliderTheme(slider, true);
        ++i;
    }

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

    for (auto j = 0; j < kEQBands.size(); ++j) {
        AddBand(num_band_);
        ++num_band_;
    }

    (void)QObject::connect(ui_.eqPresetComboBox, &QComboBox::textActivated, [this](auto index) {
        AppEQSettings settings;
        settings.name = index;
        settings.settings = AppSettings::GetEqPreset()[index];
        AppSettings::SetEqSettings(settings);
        AppSettings::save();
        ApplySetting(index, settings.settings);
        });

    if (AppSettings::contains(kAppSettingEQName)) {
        auto [name, settings] = AppSettings::GetEqSettings();
        ui_.eqPresetComboBox->setCurrentText(name);
        ApplySetting(name, settings);
    }

    (void)QObject::connect(ui_.newButton, &QPushButton::pressed, [this]() {
        ui_.eqPresetComboBox->addItem(tr("Manual"));
        ui_.eqPresetComboBox->setCurrentText(tr("Manual"));
        for (auto i = 0; i < kEQBands.size(); ++i) {
            SetBand(i, EQFilterTypes::FT_ALL_PASS, kEQBands[i], 0, kDefaultQ, kEQBands[i] / 2);
            Save(i);
        }
        const auto settings = AppSettings::GetEqPreset()[tr("Manual")];
        ApplySetting(tr("Manual"), settings);
        });

    (void)QObject::connect(ui_.removeButton, &QPushButton::pressed, [this]() {
        --num_band_;
        RemoveBand(num_band_);
        });

    (void)QObject::connect(parametric_eq_view_, &ParametricEqView::DataChanged, [this](auto freq, auto val) {
        for (auto i = 0; i < kEQBands.size(); ++i) {
            if (kEQBands[i] == freq) {
                SetBandGain(i, val);
                Save(i);
                break;
            }
        }
        });

    (void)QObject::connect(ui_.saveButton, &QPushButton::pressed, [this]() {
        for (auto i = 0; i < kEQBands.size(); ++i) {
            Save(i);
        }
        });

    (void)QObject::connect(ui_.resetButton, &QPushButton::pressed, [this]() {
        AppEQSettings settings;
        settings.name = ui_.eqPresetComboBox->currentText();
        for (auto& band : settings.settings.bands) {
	        band.Q = kDefaultQ;
        }
        ApplySetting(settings.name, settings.settings);
        AppSettings::SetEqSettings(settings);
        AppSettings::save();
        });

    ui_.sliderFrame->hide();

    /*timer_.setTimerType(Qt::PreciseTimer);
    timer_.start(100);
    (void)QObject::connect(&timer_, &QTimer::timeout, [this]() {
        for (auto i = 0; i < fft_result_.size(); ++i) {
            auto value = ToDb(fft_result_[i]);
            auto freq = Index2Freq(i, 44100, 4096);
            parametric_eq_view_->SetSpectrumData(freq, value);
        }
        parametric_eq_view_->replot(QCustomPlot::rpQueuedReplot);
        });*/
}

void EqualizerDialog::Save(uint32_t band) {
    AppEQSettings settings;
    settings.name = ui_.eqPresetComboBox->currentText();
    settings.settings = AppSettings::GetEqPreset()[settings.name];
    settings.settings.bands[band].gain = filter_settings_[band].gain->text().toFloat();
    settings.settings.bands[band].Q = filter_settings_[band].Q->text().toFloat();
    settings.settings.preamp = ui_.preampSlider->value() / 10.0;
    band_sliders_[band]->setValue(settings.settings.bands[band].gain * 10);
    AppSettings::SetEqSettings(settings);
    AppSettings::save();
}

void EqualizerDialog::OnFftResultChanged(const ComplexValarray& result) {
    fft_result_ = result;
}

void EqualizerDialog::ApplySetting(QString const& name, EQSettings const& settings) {
    for (size_t i = 0; i < settings.bands.size(); ++i) {
        band_sliders_[i]->setValue(settings.bands[i].gain * 10);
        band_label_[i]->setText(FormatDb(settings.bands[i].gain));
        SetBand(i, 
            settings.bands[i].type,
            kEQBands[i],
            settings.bands[i].gain,
            settings.bands[i].Q,
            kEQBands[i] / 2);
    }
    ui_.preampSlider->setValue(settings.preamp * 10);
    ui_.preampDbLabel->setText(FormatDb(settings.preamp));    
}

void EqualizerDialog::RemoveBand(uint32_t band) const {
    filter_settings_[band - 1].band->hide();
    filter_settings_[band - 1].type->hide();
    filter_settings_[band - 1].frequency->hide();
    filter_settings_[band - 1].gain->hide();
    filter_settings_[band - 1].Q->hide();
    filter_settings_[band - 1].band_width->hide();
}

void EqualizerDialog::SetBandGain(uint32_t band, float gain) {
    filter_settings_[band].gain->setText(FormatDouble(gain));
    parametric_eq_view_->SetBand(kEQBands[band], gain);
}

void EqualizerDialog::SetBand(uint32_t band, EQFilterTypes type, float frequency, float gain, float Q, float band_width) {
    filter_settings_[band].band->show();
    filter_settings_[band].type->show();
    filter_settings_[band].frequency->show();
    filter_settings_[band].gain->show();
    filter_settings_[band].Q->show();
    filter_settings_[band].band_width->show();

    filter_settings_[band].type->setCurrentIndex(static_cast<int>(type));
    filter_settings_[band].frequency->setText(FormatDouble(frequency));
    filter_settings_[band].gain->setText(FormatDouble(gain));
    filter_settings_[band].Q->setText(FormatDouble(Q));
    filter_settings_[band].band_width->setText(FormatDouble(frequency / 2));
    parametric_eq_view_->SetBand(static_cast<int>(frequency), gain);
}

void EqualizerDialog::AddBand(uint32_t band) {
    auto * bandLabel = new QLabel(this);
    bandLabel->setObjectName(QString::fromUtf8("bandLabel"));
    bandLabel->setStyleSheet(qTEXT("background-color: transparent;"));
    bandLabel->setText(qSTR("Band %1").arg(band));

    ui_.gridLayout->addWidget(bandLabel, band + 1, 1, 1, 1);

    auto* typeComBox = new QComboBox(this);
	typeComBox->addItem(QString());
    typeComBox->addItem(QString());
    typeComBox->addItem(QString());
    typeComBox->addItem(QString());
    typeComBox->addItem(QString());
    typeComBox->addItem(QString());
    typeComBox->addItem(QString());
    typeComBox->addItem(QString());
    typeComBox->addItem(QString());
    typeComBox->setObjectName(QString::fromUtf8("typeComBox"));
    QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    sizePolicy.setHeightForWidth(typeComBox->sizePolicy().hasHeightForWidth());
    typeComBox->setSizePolicy(sizePolicy);
    typeComBox->setMinimumSize(QSize(100, 0));

    typeComBox->setItemText(0, tr("Low-Pass"));
	typeComBox->setItemText(1, tr("High-Pass"));
    typeComBox->setItemText(2, tr("High-Band-Pass"));
    typeComBox->setItemText(3, tr("High-Band-Pass-Q"));
    typeComBox->setItemText(4, tr("Notch"));
    typeComBox->setItemText(5, tr("All-Pass"));
    typeComBox->setItemText(6, tr("All-Peak-EQ"));
    typeComBox->setItemText(7, tr("Low-Shelf"));
    typeComBox->setItemText(8, tr("High-Low-Shelf"));

    ui_.gridLayout->addWidget(typeComBox, band + 1, 2, 1, 1);


    auto* bandWidthLineEdit = new QLineEdit(this);
    bandWidthLineEdit->setObjectName(QString::fromUtf8("bandWidthLineEdit"));
    sizePolicy.setHeightForWidth(bandWidthLineEdit->sizePolicy().hasHeightForWidth());
    bandWidthLineEdit->setSizePolicy(sizePolicy);
    bandWidthLineEdit->setMaximumSize(QSize(110, 40));
    bandWidthLineEdit->setReadOnly(true);

    ui_.gridLayout->addWidget(bandWidthLineEdit, band + 1, 9, 1, 1);

    auto* gainLineEdit = new QLineEdit(this);
    gainLineEdit->setObjectName(QString::fromUtf8("gainLineEdit"));
    sizePolicy.setHeightForWidth(gainLineEdit->sizePolicy().hasHeightForWidth());
    gainLineEdit->setSizePolicy(sizePolicy);
    gainLineEdit->setMaximumSize(QSize(110, 40));
    auto* gain_validator = new QDoubleValidator(-12, 12, 2, this);
    gain_validator->setNotation(QDoubleValidator::StandardNotation);
    gainLineEdit->setValidator(gain_validator);

    ui_.gridLayout->addWidget(gainLineEdit, band + 1, 5, 1, 1);

    auto* qLineEdit = new QLineEdit(this);
    qLineEdit->setObjectName(QString::fromUtf8("qLineEdit"));
    sizePolicy.setHeightForWidth(qLineEdit->sizePolicy().hasHeightForWidth());
    qLineEdit->setSizePolicy(sizePolicy);
    qLineEdit->setMaximumSize(QSize(110, 40));

    ui_.gridLayout->addWidget(qLineEdit, band + 1, 7, 1, 1);

    auto* freqLineEdit = new QLineEdit(this);
    freqLineEdit->setObjectName(QString::fromUtf8("freqLineEdit"));
    sizePolicy.setHeightForWidth(freqLineEdit->sizePolicy().hasHeightForWidth());
    freqLineEdit->setSizePolicy(sizePolicy);
    freqLineEdit->setMaximumSize(QSize(110, 40));

    ui_.gridLayout->addWidget(freqLineEdit, band + 1, 3, 1, 1);

    filter_settings_[band - 1].band = bandLabel;
    filter_settings_[band - 1].type = typeComBox;
    filter_settings_[band - 1].band_width = bandWidthLineEdit;
    filter_settings_[band - 1].gain = gainLineEdit;
    filter_settings_[band - 1].Q = qLineEdit;
    filter_settings_[band - 1].frequency = freqLineEdit;

    auto index = band - 1;
    (void) QObject::connect(gainLineEdit, &QLineEdit::editingFinished, [index, gainLineEdit, this]() {
        SetBandGain(index, gainLineEdit->text().toFloat());
        });

    const auto f = qTheme.MonoFont();
    filter_settings_[band - 1].type->setFont(f);
    filter_settings_[band - 1].frequency->setFont(f);
    filter_settings_[band - 1].gain->setFont(f);
    filter_settings_[band - 1].Q->setFont(f);
    filter_settings_[band - 1].band_width->setFont(f);
}
