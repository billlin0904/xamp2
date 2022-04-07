#include <QFileDialog>
#include <QDir>
#include <QInputDialog>

#include <stream/podcastcache.h>

#include <widget/str_utilts.h>
#include <widget/ui_utilts.h>
#include <widget/appsettings.h>
#include <widget/localelanguage.h>
#include <widget/jsonsettings.h>
#include <widget/pixmapcache.h>
#include <preferencepage.h>

void PreferencePage::updateSoxrConfigUI(const QVariantMap& soxr_settings) {
	ui_.soxrTargetSampleRateComboBox->setCurrentText(QString::number(soxr_settings[kSoxrResampleSampleRate].toInt()));
	ui_.soxrResampleQualityComboBox->setCurrentIndex(soxr_settings[kSoxrQuality].toInt());
	ui_.soxrPassbandSlider->setValue(soxr_settings[kSoxrPassBand].toInt());
	ui_.soxrPassbandValue->setText(QString(Q_UTF8("%0%")).arg(ui_.soxrPassbandSlider->value()));
    ui_.soxrPhaseSlider->setValue(soxr_settings[kSoxrPhase].toInt());
	setPhasePercentText(ui_.soxrPhaseSlider->value());
    ui_.rollOffLevelComboBox->setCurrentIndex(soxr_settings[kSoxrRollOffLevel].toInt());

	if (soxr_settings[kSoxrEnableSteepFilter].toBool()) {
		ui_.enableSteepFilterBox->setChecked(true);
	}
	else {
		ui_.enableSteepFilterBox->setChecked(false);
	}
}

QMap<QString, QVariant> PreferencePage::currentSoxrSettings() const {
	const auto soxr_sample_rate = ui_.soxrTargetSampleRateComboBox->currentText().toInt();
	const auto soxr_quility = ui_.soxrResampleQualityComboBox->currentIndex();
	const auto soxr_pass_band = ui_.soxrPassbandSlider->value();
	const auto soxr_phase = ui_.soxrPhaseSlider->value();
    const auto soxr_rolloff = ui_.rollOffLevelComboBox->currentIndex();
	const auto soxr_enable_steep_filter = ui_.enableSteepFilterBox->checkState() == Qt::Checked;

	QMap<QString, QVariant> settings;
	settings[kSoxrResampleSampleRate] = soxr_sample_rate;
	settings[kSoxrEnableSteepFilter] = soxr_enable_steep_filter;
    settings[kSoxrRollOffLevel] = soxr_rolloff;
    settings[kSoxrQuality] = soxr_quility;
	settings[kSoxrPassBand] = soxr_pass_band;
	settings[kSoxrPhase] = soxr_phase;
	// todo: kSoxrStopBand always set 100?
	settings[kSoxrStopBand] = 100;

	return settings;
}

void PreferencePage::saveSoxrResampler(const QString &name) const {
	QMap<QString, QVariant> soxr_settings;
	soxr_settings[name] = currentSoxrSettings();

	auto soxr_config= JsonSettings::getValueAsMap(kSoxr);

	JsonSettings::setValue(kSoxr, soxr_settings);
    AppSettings::setValue(kAppSettingSoxrSettingName, name);

	if (!soxr_config.contains(name)) {
		ui_.soxrSettingCombo->addItem(name);
	}
}

void PreferencePage::initSoxResampler() {
	auto soxr_config = JsonSettings::getValueAsMap(kSoxr);

    Q_FOREACH (const auto &soxr_setting_name, soxr_config.keys()) {
		ui_.soxrSettingCombo->addItem(soxr_setting_name);
	}

    const auto enable_resampler = AppSettings::getValueAsBool(kAppSettingResamplerEnable);
	if (!enable_resampler) {
		ui_.resamplerStackedWidget->setCurrentIndex(0);
		ui_.selectResamplerComboBox->setCurrentIndex(0);
	}
	else {
		ui_.resamplerStackedWidget->setCurrentIndex(1);
		ui_.selectResamplerComboBox->setCurrentIndex(1);
	}
	
    const auto soxr_settings = soxr_config[AppSettings::getValueAsString(kAppSettingSoxrSettingName)].toMap();
	updateSoxrConfigUI(soxr_settings);

    ui_.enableFramelessWindowPushButton->setSwitchOn(AppSettings::getValueAsBool(kAppSettingUseFramelessWindow));
    (void)QObject::connect(ui_.enableFramelessWindowPushButton, &SwitchButton::pressed, [this]() {
        AppSettings::setValue(kAppSettingUseFramelessWindow,
                              !AppSettings::getValueAsBool(kAppSettingUseFramelessWindow));
    });

    (void)QObject::connect(ui_.saveSoxrSettingBtn, &QPushButton::pressed, [this]() {
	    const auto setting_name = QInputDialog::getText(this, tr("Save soxr setting"), 
	                                                    tr("Setting name:"),
	                                                    QLineEdit::Normal,
	                                                    ui_.soxrSettingCombo->currentText());
        if (setting_name.isEmpty()) {
            return;
        }
        saveSoxrResampler(setting_name);
		});

	(void)QObject::connect(ui_.deleteSoxrSettingBtn, &QPushButton::pressed, [this]() {
		const auto name = ui_.soxrSettingCombo->currentText();
		if (name == kSoxrDefaultSettingName) {
			return;
		}
        JsonSettings::remove(name);
        ui_.soxrSettingCombo->removeItem(ui_.soxrSettingCombo->currentIndex());
		saveSoxrResampler(ui_.soxrSettingCombo->currentText());
		});

    (void)QObject::connect(ui_.soxrTargetSampleRateComboBox, &QComboBox::textActivated, [this](auto) {
		saveSoxrResampler(ui_.soxrSettingCombo->currentText());
		});

    (void)QObject::connect(ui_.rollOffLevelComboBox, &QComboBox::textActivated, [this](auto) {
        saveSoxrResampler(ui_.soxrSettingCombo->currentText());
    });

    (void)QObject::connect(ui_.soxrSettingCombo, &QComboBox::textActivated, [this](auto index) {
		const auto soxr_settings = JsonSettings::getValueAsMap(kSoxr);
		const auto settings = soxr_settings[index].toMap();
		updateSoxrConfigUI(settings);
		});
}

void PreferencePage::setLang(int index) {
	auto lang = LocaleLanguageManager::languageNames()[index];
	ui_.langCombo->setCurrentIndex(index);
	AppSettings::loadLanguage(lang.getIsoCode());
	AppSettings::setValue(kAppSettingLang, lang.getIsoCode());
	ui_.retranslateUi(this);
}

void PreferencePage::initLang() {
	const LocaleLanguage current_lang(AppSettings::getValueAsString(kAppSettingLang));

	auto current_index = 0;
	auto index = 0;
    Q_FOREACH (auto lang, LocaleLanguageManager::languageNames()) {
		auto file_name = Q_STR(":/xamp/Resource/Flags/%1.png").arg(lang.countryIsoCode().toUpper());
        QIcon ico(file_name);
		ui_.langCombo->addItem(ico, lang.nativeNameLang());
		if (current_lang.getIsoCode() == lang.getIsoCode()) {
			current_index = index;
		}
		index++;
	}

    ui_.langCombo->setCurrentIndex(current_index);

	(void)QObject::connect(ui_.langCombo, static_cast<void (QComboBox::*)(int32_t)>(&QComboBox::activated), 
		[this](auto const& index) {
		setLang(index);
		});
}

void PreferencePage::setPhasePercentText(int32_t value) {
	auto str = QString(Q_UTF8("%0%")).arg(value);
	if (value == 0) {
		str += tr(" (minimum)");
	}
	else if (value < 50 && value != 0) {
		str += tr(" (intermediate)");
	}
	else if (value == 50) {
		str += tr(" (linear)");
	}
	ui_.soxrPhaseValue->setText(str);
}

PreferencePage::PreferencePage(QWidget *parent)
    : QFrame(parent) {
    ui_.setupUi(this);
    setStyleSheet(Q_UTF8("#PreferenceDialog { background-color: transparent }"));

	Singleton<ThemeManager>::GetInstance().setMenuStyle(ui_.langCombo->view()->window());
	Singleton<ThemeManager>::GetInstance().setMenuStyle(ui_.replayGainModeCombo->view()->window());

	Singleton<ThemeManager>::GetInstance().setMenuStyle(ui_.selectResamplerComboBox->view()->window());
	Singleton<ThemeManager>::GetInstance().setMenuStyle(ui_.soxrSettingCombo->view()->window());
	Singleton<ThemeManager>::GetInstance().setMenuStyle(ui_.soxrTargetSampleRateComboBox->view()->window());
	Singleton<ThemeManager>::GetInstance().setMenuStyle(ui_.soxrResampleQualityComboBox->view()->window());
	Singleton<ThemeManager>::GetInstance().setMenuStyle(ui_.rollOffLevelComboBox->view()->window());

    ui_.preferenceTreeWidget->header()->hide();
    ui_.preferenceTreeWidget->setStyleSheet(Q_UTF8("QTreeView { background: transparent; }"));

    auto* playback_item = new QTreeWidgetItem(QStringList() << tr("Playback"));
    playback_item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);

    auto* dsp_manager_item = new QTreeWidgetItem(QStringList() << tr("Resampler"));
    playback_item->addChild(dsp_manager_item);

	auto* flac_encoding_item = new QTreeWidgetItem(QStringList() << tr("Flac Encoding"));
	playback_item->addChild(flac_encoding_item);

    ui_.preferenceTreeWidget->addTopLevelItem(playback_item);
    ui_.preferenceTreeWidget->expandAll();

    (void) QObject::connect(ui_.preferenceTreeWidget, &QTreeWidget::itemClicked, [this](auto item, auto column) {
        const std::map<QString, int32_t> stack_page_map{
            { tr("Playback"), 0 },
            { tr("Resampler"), 1 },
			{ tr("Flac Encoding"), 2 },
        };

	    const auto select_type = item->text(column);
		const auto itr = stack_page_map.find(select_type);

        if (itr != stack_page_map.end()) {
            ui_.stackedWidget->setCurrentIndex((*itr).second);
        } else {
            ui_.stackedWidget->setCurrentIndex(0);
        }
    });

	ui_.selectResamplerComboBox->removeItem(2);

    (void)QObject::connect(ui_.replayGainModeCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), [](auto index) {
		switch (index) {
		case 0:
			AppSettings::setEnumValue(kAppSettingReplayGainMode, ReplayGainMode::RG_ALBUM_MODE);
			AppSettings::setValue(kAppSettingEnableReplayGain, true);
			break;
		case 1:
			AppSettings::setEnumValue(kAppSettingReplayGainMode, ReplayGainMode::RG_TRACK_MODE);
			AppSettings::setValue(kAppSettingEnableReplayGain, true);
			break;
		case 2:
			AppSettings::setEnumValue(kAppSettingReplayGainMode, ReplayGainMode::RG_NONE_MODE);
			AppSettings::setValue(kAppSettingEnableReplayGain, false);			
			break;
		}
		});

	ui_.replayGainModeCombo->setCurrentIndex(AppSettings::getValue(kAppSettingReplayGainMode).toInt());

	(void)QObject::connect(ui_.selectResamplerComboBox, static_cast<void (QComboBox::*)(int32_t)>(&QComboBox::activated), [this](auto const& index) {
		ui_.resamplerStackedWidget->setCurrentIndex(index);
		saveSoxrResampler(ui_.soxrSettingCombo->currentText());
        saveAll();
		});

    (void)QObject::connect(ui_.soxrPassbandSlider, &QSlider::valueChanged, [this](auto) {
        ui_.soxrPassbandValue->setText(QString(Q_UTF8("%0%")).arg(ui_.soxrPassbandSlider->value()));
		saveSoxrResampler(ui_.soxrSettingCombo->currentText());
        saveAll();
    });

    (void)QObject::connect(ui_.soxrPhaseSlider, &QSlider::valueChanged, [this](auto) {
		setPhasePercentText(ui_.soxrPhaseSlider->value());
		saveSoxrResampler(ui_.soxrSettingCombo->currentText());
        saveAll();
    });

	(void)QObject::connect(ui_.resetAllButton, &QPushButton::clicked, [this]() {
		initSoxResampler();
        saveAll();
		});

	ui_.podcastCachePathLineEdit->setText(AppSettings::getValue(kAppSettingPodcastCachePath).toString());
	(void)QObject::connect(ui_.setPodcastCacheButton, &QPushButton::clicked, [this]() {
		const auto dir_name = QFileDialog::getExistingDirectory(this,
			tr("Select a directory"),
			AppSettings::getMyMusicFolderPath(),
            QFileDialog::ShowDirsOnly);
		if (dir_name.isEmpty()) {
			return;
		}
		PodcastCache.SetTempPath(dir_name.toStdWString());
		AppSettings::setValue(kAppSettingPodcastCachePath, dir_name);
		ui_.podcastCachePathLineEdit->setText(dir_name);
		});

    ui_.albumImageCacheSizeSpinBox->setValue(AppSettings::getValue(kAppSettingAlbumImageCacheSize).toInt());
    (void)QObject::connect(ui_.albumImageCacheSizeSpinBox, static_cast<void (QSpinBox::*)(int32_t)>(&QSpinBox::valueChanged), [](auto value) {
        Singleton<PixmapCache>::GetInstance().setMaxSize(value);
        AppSettings::setValue(kAppSettingAlbumImageCacheSize, value);
    });

	initSoxResampler();
	initLang();

	ui_.flacCompressionLevelSlider->setValue(AppSettings::getValue(kFlacEncodingLevel).toInt());
	(void)QObject::connect(ui_.flacCompressionLevelSlider, &QSlider::valueChanged, [this](auto value) {
		AppSettings::setValue(kFlacEncodingLevel, value);
		saveAll();
		});
}

void PreferencePage::update() {
	auto enable_resampler = AppSettings::getValueAsBool(kAppSettingResamplerEnable);
	if (!enable_resampler) {
		ui_.resamplerStackedWidget->setCurrentIndex(0);
		ui_.selectResamplerComboBox->setCurrentIndex(0);
	}
	else {
		ui_.resamplerStackedWidget->setCurrentIndex(1);
		ui_.selectResamplerComboBox->setCurrentIndex(1);
	}
}

void PreferencePage::saveAll() {
	saveSoxrResampler(ui_.soxrSettingCombo->currentText());
	AppSettings::setValue(kFlacEncodingLevel, ui_.flacCompressionLevelSlider->value());

	auto index = ui_.resamplerStackedWidget->currentIndex();
	AppSettings::setValue(kAppSettingResamplerEnable, index > 0);

	JsonSettings::save();
	AppSettings::save();
}
