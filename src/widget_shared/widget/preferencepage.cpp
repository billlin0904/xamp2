#include <widget/windowbackdrop.h>
#include <QCheckBox>
#include <QTabBar>
#include <widget/preferencepage.h>
#include <ui_preferencedialog.h>
#include <QInputDialog>

#include <widget/util/str_util.h>
#include <widget/util/ui_util.h>
#include <widget/appsettings.h>
#include <widget/localelanguage.h>
#include <widget/jsonsettings.h>
#include <widget/imagecache.h>
#include <widget/widget_shared.h>
#include <widget/dao/dbfacade.h>

#include <thememanager.h>

void PreferencePage::updateSoxrConfigUi(const QVariantMap& soxr_settings) {
	ui_->soxrTargetSampleRateComboBox->setCurrentText(QString::number(soxr_settings[kResampleSampleRate].toInt()));
	ui_->soxrResampleQualityComboBox->setCurrentIndex(soxr_settings[kSoxrQuality].toInt());
	ui_->soxrPassbandSlider->setValue(soxr_settings[kSoxrPassBand].toInt());
	ui_->soxrPassbandValue->setText(qFormat("%0%").arg(ui_->soxrPassbandSlider->value()));
    ui_->soxrPhaseSlider->setValue(soxr_settings[kSoxrPhase].toInt());
	setPhasePercentText(ui_->soxrPhaseSlider->value());
    ui_->rollOffLevelComboBox->setCurrentIndex(soxr_settings[kSoxrRollOffLevel].toInt());
}

QMap<QString, QVariant> PreferencePage::currentSoxrSettings() const {
	const auto soxr_sample_rate = ui_->soxrTargetSampleRateComboBox->currentText().toInt();
	const auto soxr_quility = ui_->soxrResampleQualityComboBox->currentIndex();
	const auto soxr_pass_band = ui_->soxrPassbandSlider->value();
	const auto soxr_phase = ui_->soxrPhaseSlider->value();
    const auto soxr_rolloff = ui_->rollOffLevelComboBox->currentIndex();
	const auto soxr_enable_steep_filter = false;

	QMap<QString, QVariant> settings;
	settings[kResampleSampleRate] = soxr_sample_rate;
    settings[kSoxrRollOffLevel] = soxr_rolloff;
    settings[kSoxrQuality] = soxr_quility;
	settings[kSoxrPassBand] = soxr_pass_band;
	settings[kSoxrPhase] = soxr_phase;
	// TODO: kSoxrStopBand always set 100?
	settings[kSoxrStopBand] = 100;

	return settings;
}

void PreferencePage::saveSoxrResampler(const QString &name) const {
	auto soxr_config = qJsonSettings.valueAsMap(kSoxr);

	soxr_config[name] = currentSoxrSettings();
	qJsonSettings.setValue(kSoxr, soxr_config);
	qAppSettings.setValue(kAppSettingSoxrSettingName, name);

	if (!soxr_config.contains(name)) {
		ui_->soxrSettingCombo->addItem(name);
	}
}

void PreferencePage::saveSrcResampler() {
	QMap<QString, QVariant> settings;
	settings[kResampleSampleRate] = ui_->srcTargetSampleRateComboBox->currentText().toInt();
	qJsonSettings.setValue(kSrc, settings);
}

void PreferencePage::saveR8BrainResampler() {
	QMap<QString, QVariant> settings;
	settings[kResampleSampleRate] = ui_->r8brainTargetSampleRateComboBox->currentText().toInt();
	qJsonSettings.setValue(kR8Brain, settings);
}

void PreferencePage::initSrcResampler() {
	auto config = qJsonSettings.valueAsMap(kSrc);
	ui_->srcTargetSampleRateComboBox->setCurrentText(QString::number(config[kResampleSampleRate].toInt()));

	(void)QObject::connect(ui_->srcTargetSampleRateComboBox, &QComboBox::textActivated, [this](auto) {
		saveSrcResampler();
		});
}

void PreferencePage::initR8BrainResampler() {
	auto config = qJsonSettings.valueAsMap(kR8Brain);
	ui_->r8brainTargetSampleRateComboBox->setCurrentText(QString::number(config[kResampleSampleRate].toInt()));

	(void)QObject::connect(ui_->r8brainTargetSampleRateComboBox, &QComboBox::textActivated, [this](auto) {
		saveR8BrainResampler();
		});	
}

void PreferencePage::initSoxResampler() {
	auto soxr_config = qJsonSettings.valueAsMap(kSoxr);

    Q_FOREACH (const auto &soxr_setting_name, soxr_config.keys()) {
		ui_->soxrSettingCombo->addItem(soxr_setting_name);
	}
	
    const auto soxr_settings = soxr_config[qAppSettings.valueAsString(kAppSettingSoxrSettingName)].toMap();
	updateSoxrConfigUi(soxr_settings);

    (void)QObject::connect(ui_->saveSoxrSettingBtn, &QPushButton::pressed, [this]() {
		if (!ui_->soxrSettingCombo->count()) {
			return;
		}
		auto setting_name = ui_->soxrSettingCombo->currentText();
        saveSoxrResampler(setting_name);
		});

	(void)QObject::connect(ui_->deleteSoxrSettingBtn, &QPushButton::pressed, [this]() {
		const auto name = ui_->soxrSettingCombo->currentText();
		if (name == kSoxrDefaultSettingName) {
			return;
		}
        qJsonSettings.remove(name);
        ui_->soxrSettingCombo->removeItem(ui_->soxrSettingCombo->currentIndex());
		saveSoxrResampler(ui_->soxrSettingCombo->currentText());
		});

    (void)QObject::connect(ui_->soxrTargetSampleRateComboBox, &QComboBox::textActivated, [this](auto) {
		saveSoxrResampler(ui_->soxrSettingCombo->currentText());
		});

    (void)QObject::connect(ui_->rollOffLevelComboBox, &QComboBox::textActivated, [this](auto) {
        saveSoxrResampler(ui_->soxrSettingCombo->currentText());
    });

    (void)QObject::connect(ui_->soxrSettingCombo, &QComboBox::textActivated, [this](auto index) {
		const auto soxr_settings = qJsonSettings.valueAsMap(kSoxr);
		const auto settings = soxr_settings[index].toMap();
		updateSoxrConfigUi(settings);
		});
}

void PreferencePage::setLanguage(int index) {
	const auto lang = LocaleLanguageManager::languageNames()[index];
	ui_->langCombo->setCurrentIndex(index);
	qAppSettings.loadLanguage(lang.isoCode());
	qAppSettings.setValue(kAppSettingLang, lang.isoCode());
	ui_->retranslateUi(this);
	emit retranslateUi();
}

void PreferencePage::initialLanguage() {
	const LocaleLanguage current_lang(qAppSettings.valueAsString(kAppSettingLang));

	auto current_index = 0;
	auto index = 0;

    Q_FOREACH (auto lang, LocaleLanguageManager::languageNames()) {
        QIcon ico(qTheme.countryFlagFilePath(lang.countryIsoCode().toUpper()));
		ui_->langCombo->addItem(ico, lang.nativeNameLang());
		if (current_lang.isoCode() == lang.isoCode()) {
			current_index = index;
		}
		index++;
	}

    ui_->langCombo->setCurrentIndex(current_index);
    if (ui_->langCombo->count() == 0) {
        ui_->langCombo->addItem(current_lang.nativeNameLang());
        ui_->langCombo->setEnabled(false);
    }

	(void)QObject::connect(ui_->langCombo, static_cast<void (QComboBox::*)(int32_t)>(&QComboBox::activated), 
		[this](auto const& index) {
		setLanguage(index);
		});
}

void PreferencePage::setPhasePercentText(int32_t value) {
	auto str = qFormat("%0%").arg(value);
	if (value == 0) {
		str += tr(" (minimum)");
	}
	else if (value < 50 && value != 0) {
		str += tr(" (intermediate)");
	}
	else if (value == 50) {
		str += tr(" (linear)");
	}
	ui_->soxrPhaseValue->setText(str);
}

PreferencePage::PreferencePage(QWidget *parent)
    : QFrame(parent) {
	ui_ = new Ui::PreferenceDialog();
    ui_->setupUi(this);
    auto* bitperfect = new QCheckBox(tr("BitPerfect PCM (WASAPI Exclusive / ASIO)"), this);
    bitperfect->setObjectName(QStringLiteral("bitPerfectCheckBox"));
    bitperfect->setChecked(qAppSettings.valueAsBool(QStringLiteral("bitPerfectEnabled")));
    auto* bitperfect_hint = new QLabel(tr("Next playback: stereo 16/24/32-bit integer WAV or FLAC at the original sample rate. Requires an integer WASAPI Exclusive or ASIO output with enough valid bits. Bypasses DSP and software volume; use your DAC volume control."), this);
    bitperfect_hint->setWordWrap(true);
    auto* bitperfect_row = new QFrame(this);
    bitperfect_row->setProperty("settingRow", true);
    auto* bitperfect_layout = new QVBoxLayout(bitperfect_row);
    bitperfect_layout->setContentsMargins(18, 14, 18, 14);
    bitperfect_layout->addWidget(bitperfect);
    bitperfect_layout->addWidget(bitperfect_hint);
    auto* output_page = new QWidget(this);
    output_page->setObjectName(QStringLiteral("outputSettingsPage"));
    auto* output_layout = new QVBoxLayout(output_page);
    output_layout->setContentsMargins(0, 0, 0, 0);
    output_layout->setSpacing(12);
    output_layout->addWidget(bitperfect_row);
    output_layout->addStretch();
    ui_->stackedWidget->addWidget(output_page);
    connect(this, &PreferencePage::retranslateUi, this, [bitperfect, bitperfect_hint] {
        bitperfect->setText(tr("BitPerfect PCM (WASAPI Exclusive / ASIO)"));
        bitperfect_hint->setText(tr("Next playback: stereo 16/24/32-bit integer WAV or FLAC at the original sample rate. Requires an integer WASAPI Exclusive or ASIO output with enough valid bits. Bypasses DSP and software volume; use your DAC volume control."));
    });
    connect(bitperfect, &QCheckBox::toggled, this, [](bool enabled) {
        qAppSettings.setValue(QStringLiteral("bitPerfectEnabled"), enabled);
    });
    ui_->backdropCombo->setCurrentIndex(WindowBackdrop::mode());
    ui_->backdropCombo->setEnabled(WindowBackdrop::supported());
    if (!WindowBackdrop::supported()) {
        ui_->backdropHint->setText(tr("Requires Windows 11 22H2 or later. Uses a solid background here."));
    }
    connect(ui_->backdropCombo, &QComboBox::currentIndexChanged,
        this, [](int index) { WindowBackdrop::setMode(index); });

	initSoxResampler();
	initR8BrainResampler();
	initSrcResampler();

	initialLanguage();

	switch (qTheme.themeColor()) {
	case ThemeColor::LIGHT_THEME:
		ui_->lightRadioButton->setChecked(true);
		break;
	case ThemeColor::DARK_THEME:
		ui_->darkRadioButton->setChecked(true);
		break;
	}

	(void)QObject::connect(ui_->lightRadioButton, &QRadioButton::clicked, [](auto checked) {
		qTheme.setThemeColor(ThemeColor::LIGHT_THEME);
		emit qTheme.themeChangedFinished(ThemeColor::LIGHT_THEME);		
		});

	(void)QObject::connect(ui_->darkRadioButton, &QRadioButton::clicked, [](auto checked) {
		qTheme.setThemeColor(ThemeColor::DARK_THEME);
		emit qTheme.themeChangedFinished(ThemeColor::DARK_THEME);		
		});

	(void)QObject::connect(ui_->selectResamplerComboBox, static_cast<void (QComboBox::*)(int32_t)>(&QComboBox::activated), [this](auto const& index) {
		ui_->resamplerStackedWidget->setCurrentIndex(index);
		switch (index) {
		case 1:
			saveSoxrResampler(ui_->soxrSettingCombo->currentText());
			qAppSettings.setValue(kAppSettingResamplerType, kSoxr);
			break;
		case 2:
			saveSrcResampler();
			qAppSettings.setValue(kAppSettingResamplerType, kSrc);
			break;
		case 3:
			saveR8BrainResampler();
			qAppSettings.setValue(kAppSettingResamplerType, kR8Brain);
			break;
		}		
        saveAll();
		});

	auto replayGainMode = qAppSettings.valueAsEnum<ReplayGainMode>(kAppSettingReplayGainMode);
	if (replayGainMode < ReplayGainMode::RG_MAX_MODE) {
		ui_->replayGainModeCombo->setCurrentIndex(std::bit_cast<int32_t>(replayGainMode));
	}

	(void)QObject::connect(ui_->replayGainModeCombo, static_cast<void (QComboBox::*)(int32_t)>(&QComboBox::activated), [this](auto const& index) {
		switch (index) {
		case 0: // Album
			qAppSettings.setEnumValue(kAppSettingReplayGainMode, ReplayGainMode::RG_ALBUM_MODE);
			break;
		case 1: // Track
			qAppSettings.setEnumValue(kAppSettingReplayGainMode, ReplayGainMode::RG_TRACK_MODE);
			break;
		case 2: // None
			qAppSettings.setEnumValue(kAppSettingReplayGainMode, ReplayGainMode::RG_NONE_MODE);
			break;
		}
		});

    (void)QObject::connect(ui_->soxrPassbandSlider, &QSlider::valueChanged, [this](auto) {
        ui_->soxrPassbandValue->setText(qFormat("%0%").arg(ui_->soxrPassbandSlider->value()));
		saveSoxrResampler(ui_->soxrSettingCombo->currentText());
        saveAll();
    });

    (void)QObject::connect(ui_->soxrPhaseSlider, &QSlider::valueChanged, [this](auto) {
		setPhasePercentText(ui_->soxrPhaseSlider->value());
		saveSoxrResampler(ui_->soxrSettingCombo->currentText());
        saveAll();
    });

	(void)QObject::connect(ui_->clearCoverCacheButton, &QPushButton::clicked, [this](auto) {
		qImageCache.clearCache();
		qDaoFacade.music_dao.removeCoverId();
		});

	(void)QObject::connect(ui_->newSoxrSettingBtn, &QRadioButton::clicked, [this](auto checked) {
		const auto setting_name = QInputDialog::getText(this, tr("New soxr setting"),
		tr("Setting name:"),
		QLineEdit::Normal,
		ui_->soxrSettingCombo->currentText());
		if (setting_name.isEmpty()) {
			return;
		}
		auto soxr_config = qJsonSettings.valueAsMap(kSoxr);
		if (soxr_config.contains(setting_name)) {
			return;
		}
		saveSoxrResampler(setting_name);
		});

	(void)QObject::connect(ui_->resetAllButton, &QPushButton::clicked, [this]() {
		qJsonSettings.remove(kSoxr);
		initSoxResampler();
        saveAll();
		});

	const QList<QWidget*> widgets {
		ui_->darkRadioButton,
		ui_->lightRadioButton,
		ui_->lbThemeMode,
		ui_->lbLang,
		ui_->lbReplayGameMode,
	};

	const QList<QWidget*> soxr_page_widgets{
		ui_->lbResampler,
		ui_->lbResamplerSettings,
		ui_->lbTargetSampleRate,
		ui_->lbQuality,
		ui_->soxrPassbandValue,
		ui_->soxrPhaseValue,
		ui_->lbHz,
		ui_->lbRollOffLevel,
	};

	const QList<QWidget*> r8brain_page_widgets{
		ui_->lbR8BrainTargetSampleRate,
		ui_->lbR8BrainHz,
	};

	auto f = qTheme.uiFont();
	f.setPointSize(qTheme.fontSize(9));
	setFont(f);

	Q_FOREACH(auto *w, widgets) {
		auto df = qTheme.displayFont();
		df.setWeight(QFont::DemiBold);
		df.setPointSize(qTheme.fontSize(14));

		if (dynamic_cast<QRadioButton*>(w) == nullptr && dynamic_cast<QCheckBox*>(w) == nullptr) {
			w->setFont(f);
		}
		w->setStyleSheet("background: transparent;"_str);
	}

	Q_FOREACH(auto* w, soxr_page_widgets) {
		w->setStyleSheet("background: transparent;"_str);
	}

	Q_FOREACH(auto* w, r8brain_page_widgets) {
		w->setStyleSheet("background: transparent;"_str);
	}

	ui_->soxrPhaseSlider->setRange(1, 100);
	ui_->soxrPassbandSlider->setRange(1, 100);

	qTheme.setSliderTheme(ui_->soxrPhaseSlider, true);
	qTheme.setSliderTheme(ui_->soxrPassbandSlider, true);

	ui_->stackedWidget->setCurrentIndex(0);
    ui_->settingsTabs->setExpanding(false);
    ui_->settingsTabs->addTab(tr("Playback"));
    ui_->settingsTabs->addTab(tr("Resampler"));
    ui_->settingsTabs->addTab(tr("Output"));
    connect(ui_->settingsTabs, &QTabBar::currentChanged, ui_->stackedWidget, &QStackedWidget::setCurrentIndex);
    connect(this, &PreferencePage::retranslateUi, this, [this] {
        ui_->settingsTabs->setTabText(0, tr("Playback"));
        ui_->settingsTabs->setTabText(1, tr("Resampler"));
        ui_->settingsTabs->setTabText(2, tr("Output"));
    });
}

PreferencePage::~PreferencePage() {
	delete ui_;
}

void PreferencePage::loadSettings() {
	const auto enable_resampler = qAppSettings.valueAsBool(kAppSettingResamplerEnable);
	if (!enable_resampler) {
		ui_->resamplerStackedWidget->setCurrentIndex(0);
		ui_->selectResamplerComboBox->setCurrentIndex(0);
	}
	else {
		const auto resampler_type = qAppSettings.valueAsString(kAppSettingResamplerType);
		if (resampler_type == kSoxr || resampler_type.isEmpty()) {
			ui_->resamplerStackedWidget->setCurrentIndex(1);
			ui_->selectResamplerComboBox->setCurrentIndex(1);
		}
		else if (resampler_type == kR8Brain) {
			ui_->resamplerStackedWidget->setCurrentIndex(3);
			ui_->selectResamplerComboBox->setCurrentIndex(3);
		}
		else if (resampler_type == kSrc) {
			ui_->resamplerStackedWidget->setCurrentIndex(2);
			ui_->selectResamplerComboBox->setCurrentIndex(2);
		}
	}
}

void PreferencePage::saveAll() {
	saveSoxrResampler(ui_->soxrSettingCombo->currentText());
	saveR8BrainResampler();
	saveSrcResampler();

	auto index = ui_->resamplerStackedWidget->currentIndex();
	qAppSettings.setValue(kAppSettingResamplerEnable, index > 0);

	qJsonSettings.save();
	qAppSettings.save();
}

void PreferencePage::addUpdatesPage(QWidget* page) {
    const int index = ui_->stackedWidget->addWidget(page);
    updates_page_index_ = index;
    ui_->settingsTabs->addTab(QCoreApplication::translate("ApplicationUpdater", "Software updates"));
    connect(this, &PreferencePage::retranslateUi, this, [this, index] {
        ui_->settingsTabs->setTabText(index, QCoreApplication::translate("ApplicationUpdater", "Software updates"));
    });
}

void PreferencePage::showUpdatesPage() {
    if (updates_page_index_ >= 0) ui_->settingsTabs->setCurrentIndex(updates_page_index_);
}
