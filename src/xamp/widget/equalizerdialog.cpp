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
    ui_.preampDbLabel->setStyleSheet(qTEXT("background-color: transparent;"));

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

    (void)QObject::connect(ui_.addButton, &QPushButton::pressed, [this]() {
        AddBand(num_band_, FilterTypes::FT_ALL_PASS, 0, 0, 0, 0);
        ++num_band_;
        });

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

void EqualizerDialog::AddBand(uint32_t band, FilterTypes type, float frequency, float gain, float Q, float band_width) {
    auto * label = new QLabel(this);
    label->setObjectName(QString::fromUtf8("label"));
    label->setStyleSheet(qTEXT("background-color: transparent;"));

    ui_.gridLayout->addWidget(label, 2 + band, 1, 1, 1);

    auto* comboBox = new QComboBox(this);
    comboBox->addItem(QString());
    comboBox->addItem(QString());
    comboBox->addItem(QString());
    comboBox->addItem(QString());
    comboBox->addItem(QString());
    comboBox->addItem(QString());
    comboBox->addItem(QString());
    comboBox->addItem(QString());
    comboBox->addItem(QString());
    comboBox->setObjectName(QString::fromUtf8("comboBox"));
    QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    sizePolicy.setHeightForWidth(comboBox->sizePolicy().hasHeightForWidth());
    comboBox->setSizePolicy(sizePolicy);
    comboBox->setMinimumSize(QSize(100, 0));

    comboBox->setItemText(0, tr("Low-Pass"));
    comboBox->setItemText(1, tr("High-Pass"));
    comboBox->setItemText(2, tr("High-Band-Pass"));
    comboBox->setItemText(3, tr("High-Band-Pass-Q"));
    comboBox->setItemText(4, tr("Notch"));
    comboBox->setItemText(5, tr("All-Pass"));
    comboBox->setItemText(6, tr("All-Peak-EQ"));
    comboBox->setItemText(7, tr("Low-Shelf"));
    comboBox->setItemText(8, tr("High-Low-Shelf"));

    ui_.gridLayout->addWidget(comboBox, 2 + band, 2, 1, 1);


    auto* lineEdit_4 = new QLineEdit(this);
    lineEdit_4->setObjectName(QString::fromUtf8("lineEdit_4"));
    sizePolicy.setHeightForWidth(lineEdit_4->sizePolicy().hasHeightForWidth());
    lineEdit_4->setSizePolicy(sizePolicy);
    lineEdit_4->setMaximumSize(QSize(90, 40));
    lineEdit_4->setReadOnly(true);

    ui_.gridLayout->addWidget(lineEdit_4, 2 + band, 9, 1, 1);

    auto* lineEdit_2 = new QLineEdit(this);
    lineEdit_2->setObjectName(QString::fromUtf8("lineEdit_2"));
    sizePolicy.setHeightForWidth(lineEdit_2->sizePolicy().hasHeightForWidth());
    lineEdit_2->setSizePolicy(sizePolicy);
    lineEdit_2->setMaximumSize(QSize(90, 40));

    ui_.gridLayout->addWidget(lineEdit_2, 2 + band, 5, 1, 2);

    auto* lineEdit_3 = new QLineEdit(this);
    lineEdit_3->setObjectName(QString::fromUtf8("lineEdit_3"));
    sizePolicy.setHeightForWidth(lineEdit_3->sizePolicy().hasHeightForWidth());
    lineEdit_3->setSizePolicy(sizePolicy);
    lineEdit_3->setMaximumSize(QSize(90, 40));

    ui_.gridLayout->addWidget(lineEdit_3, 2 + band, 7, 1, 1);

    auto* lineEdit = new QLineEdit(this);
    lineEdit->setObjectName(QString::fromUtf8("lineEdit"));
    sizePolicy.setHeightForWidth(lineEdit->sizePolicy().hasHeightForWidth());
    lineEdit->setSizePolicy(sizePolicy);
    lineEdit->setMaximumSize(QSize(90, 40));

    ui_.gridLayout->addWidget(lineEdit, 2 + band, 3, 1, 2);
}
