#include <widget/equalizerview.h>

#include <widget/appsettingnames.h>
#include <widget/appsettings.h>
#include <widget/str_utilts.h>
#include <widget/widget_shared.h>
#include <stream/eqsettings.h>
#include <ui_equalizerdialog.h>
#include <thememanager.h>

EqualizerView::EqualizerView(QWidget* parent)
    : QFrame(parent) {
    ui_ = new Ui::EqualizerView();
    ui_->setupUi(this);

    freq_label_ = std::vector<QLabel*>{
        ui_->band1FeqLabel,
        ui_->band2FeqLabel,
        ui_->band3FeqLabel,
        ui_->band4FeqLabel,
        ui_->band5FeqLabel,
        ui_->band6FeqLabel,
        ui_->band7FeqLabel,
        ui_->band8FeqLabel,
        ui_->band9FeqLabel,
        ui_->band10FeqLabel,
    };

    sliders_ = std::vector<DoubleSlider*>{
        ui_->band1Slider,
        ui_->band2Slider,
        ui_->band3Slider,
        ui_->band4Slider,
        ui_->band5Slider,
        ui_->band6Slider,
        ui_->band7Slider,
        ui_->band8Slider,
        ui_->band9Slider,
        ui_->band10Slider
    };

    bands_label_ = std::vector<QLabel*>{
        ui_->band1DbLabel,
        ui_->band2DbLabel,
        ui_->band3DbLabel,
        ui_->band4DbLabel,
        ui_->band5DbLabel,
        ui_->band6DbLabel,
        ui_->band7DbLabel,
        ui_->band8DbLabel,
        ui_->band9DbLabel,
        ui_->band10DbLabel,
    };

    auto f = qTheme.GetMonoFont();
    f.setPointSize(qTheme.GetFontSize(8));

    ui_->preampLabel->setFont(f);

    for (auto& l : bands_label_) {
        l->setStyleSheet(qTEXT("background-color: transparent;"));
        l->setFont(f);
    }

    for (auto& l : freq_label_) {
        l->setStyleSheet(qTEXT("background-color: transparent;"));
        l->setFont(f);
    }

    ui_->preampDbLabel->setStyleSheet(qTEXT("background-color: transparent;"));
    ui_->preampLabel->setStyleSheet(qTEXT("background-color: transparent;"));

    qTheme.SetSliderTheme(ui_->preampSlider);

    auto band = 0;
    for (auto* slider : sliders_) {
        qTheme.SetSliderTheme(slider);

        (void)QObject::connect(slider, &DoubleSlider::doubleValueChanged, [band, this](auto value) {
            bands_label_[band]->setText(FormatDb(value));
            });
        ++band;
    }

    (void)QObject::connect(ui_->resetButton, &QPushButton::clicked, [this]() {
        auto [name, settings] = qAppSettings.eqSettings();
        for (auto &band : settings.bands) {
            band.gain = 0;
        }
        settings.preamp = 0;
        applySetting(name, settings);
        });

    (void)QObject::connect(ui_->preampSlider, &DoubleSlider::doubleValueChanged, [this](auto value) {
        preampValueChanged(value);
        ui_->preampDbLabel->setText(FormatDb(value));
        });

    ui_->enableEqCheckBox->setStyleSheet(qTEXT("background-color: transparent;"));
    (void)QObject::connect(ui_->enableEqCheckBox, &QCheckBox::stateChanged, [this](auto value) {
        qAppSettings.setValue(kAppSettingEnableEQ, value == Qt::CheckState::Checked);
        });

    (void)QObject::connect(ui_->enableEqCheckBox, &QCheckBox::stateChanged, [this](auto value) {
        qAppSettings.setValue(kAppSettingEnableEQ, value == Qt::CheckState::Checked);
        });

    ui_->enableEqCheckBox->setCheckState(qAppSettings.valueAsBool(kAppSettingEnableEQ)
        ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);

    for (auto& name : qAppSettings.eqPreset().keys()) {
        ui_->eqPresetComboBox->addItem(name);
    }

    (void)QObject::connect(ui_->eqPresetComboBox, &QComboBox::textActivated, [this](auto index) {
        AppEQSettings settings;
        settings.name = index;
        settings.settings = qAppSettings.eqPreset()[index];
        if (settings.settings.bands.empty()) {
            settings.settings.bands.emplace_back();
        }
        qAppSettings.setEqSettings(settings);
        qAppSettings.save();
        applySetting(index, settings.settings);
        });

    if (qAppSettings.contains(kAppSettingEQName)) {
        auto [name, settings] = qAppSettings.eqSettings();
        AppEQSettings app_settings;
        app_settings.name = name;
        app_settings.settings = settings;
        qAppSettings.setEqSettings(app_settings);
        ui_->eqPresetComboBox->setCurrentText(name);
        applySetting(name, settings);
    }
}

EqualizerView::~EqualizerView() {
    delete ui_;
}

void EqualizerView::applySetting(const QString& name, const EqSettings& settings) {
    for (const auto& l : bands_label_) {
        l->hide();
    }
    for (const auto& l : freq_label_) {
        l->hide();
    }
    for (const auto& s : sliders_) {
        s->hide();
    }

    auto i = 0;
    if (freq_label_.size() < settings.bands.size()) {
	    const auto band_size = settings.bands.size() - freq_label_.size();

	    for (; i < band_size; ++i) {
            auto *vertical_layout = new QVBoxLayout();
            vertical_layout->setSpacing(0);
            vertical_layout->setObjectName(qSTR("verticalLayout_%1").arg(i));
            vertical_layout->setContentsMargins(1, -1, -1, -1);

            auto *band_db_label = new QLabel(this);
            band_db_label->setObjectName(qSTR("band%1DbLabel").arg(i));
            band_db_label->setAlignment(Qt::AlignCenter);

            vertical_layout->addWidget(band_db_label);

            auto* band_slider = new DoubleSlider(this);
            band_slider->setObjectName(qSTR("band%1Slider").arg(i));
            band_slider->setMinimum(-150);
            band_slider->setMaximum(150);
            band_slider->setOrientation(Qt::Vertical);
            band_slider->setTickPosition(QSlider::TicksAbove);

            vertical_layout->addWidget(band_slider);

            auto* band_feq_label = new QLabel(this);
            band_feq_label->setObjectName("band10FeqLabel");
            band_feq_label->setAlignment(Qt::AlignCenter);

            vertical_layout->addWidget(band_feq_label);

            ui_->gridLayout->addLayout(vertical_layout, 0, 10 + i, 1, 1);

            freq_label_.push_back(band_feq_label);
            bands_label_.push_back(band_db_label);
            sliders_.push_back(band_slider);
            qTheme.SetSliderTheme(band_slider);
	    }
    }

    for (i = 0; i < settings.bands.size(); ++i) {
        freq_label_[i]->setText(FormatSampleRate(settings.bands[i].frequency));
        bands_label_[i]->setText(FormatDb(settings.bands[i].gain, 2));
        sliders_[i]->setValue(settings.bands[i].gain * 10);

        freq_label_[i]->show();
        bands_label_[i]->show();
        sliders_[i]->show();
    }

    ui_->preampSlider->setValue(settings.preamp * 10);
    ui_->preampDbLabel->setText(FormatDb(settings.preamp));
}
