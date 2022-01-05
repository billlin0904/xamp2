#include <QSlider>
#include <QTreeWidget>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QDir>
#include <QInputDialog>

#include <base/uuid.h>
#include <stream/podcastcache.h>

#include <widget/str_utilts.h>
#include <widget/ui_utilts.h>
#include <widget/appsettings.h>
#include <widget/localelanguage.h>
#include <widget/jsonsettings.h>
#include <widget/pixmapcache.h>
#include "thememanager.h"
#include <preferencepage.h>

void PreferencePage::loadSoxrResampler(const QVariantMap& soxr_settings) {
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

QMap<QString, QVariant> PreferencePage::getSoxrSettings() const {
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

void PreferencePage::saveSoxrResampler(const QString &name) {
    JsonSettings::setValue(name, getSoxrSettings());
    AppSettings::setValue(kAppSettingSoxrSettingName, ui_.soxrSettingCombo->currentText());
}

void PreferencePage::initSoxResampler() {
    Q_FOREACH (const auto &soxr_setting_name, JsonSettings::keys()) {
		ui_.soxrSettingCombo->addItem(soxr_setting_name);
	}

	auto enable_resampler = AppSettings::getValueAsBool(kAppSettingResamplerEnable);
	if (!enable_resampler) {
		ui_.resamplerStackedWidget->setCurrentIndex(0);
		ui_.selectResamplerComboBox->setCurrentIndex(0);
	}
	else {
		ui_.resamplerStackedWidget->setCurrentIndex(1);
		ui_.selectResamplerComboBox->setCurrentIndex(1);
	}

	auto soxr_settings = QVariant::fromValue(
		JsonSettings::getValue(AppSettings::getValueAsString(kAppSettingSoxrSettingName))
	).toMap();

	loadSoxrResampler(soxr_settings);

    ui_.enableFramelessWindowPushButton->setSwitchOn(AppSettings::getValueAsBool(kAppSettingUseFramelessWindow));
    (void)QObject::connect(ui_.enableFramelessWindowPushButton, &SwitchButton::pressed, [this]() {
        AppSettings::setValue(kAppSettingUseFramelessWindow,
                              !AppSettings::getValueAsBool(kAppSettingUseFramelessWindow));
        saveAll();
    });

    (void)QObject::connect(ui_.saveSoxrSettingBtn, &QPushButton::pressed, [this]() {
        auto setting_name = QInputDialog::getText(this, tr("Save soxr setting"), 
			tr("Setting name:"),
			QLineEdit::Normal,
			ui_.soxrSettingCombo->currentText());
        if (setting_name.isEmpty()) {
            return;
        }
        saveSoxrResampler(setting_name);
		saveAll();
		});

	(void)QObject::connect(ui_.deleteSoxrSettingBtn, &QPushButton::pressed, [this]() {
		auto name = ui_.soxrSettingCombo->currentText();
		if (name == kSoxrDefaultSettingName) {
			return;
		}
        JsonSettings::remove(name);
        ui_.soxrSettingCombo->removeItem(ui_.soxrSettingCombo->currentIndex());
		saveAll();
		});

	(void)QObject::connect(ui_.soxrTargetSampleRateComboBox, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::activated), [this](auto index) {
		saveAll();
		});

    (void)QObject::connect(ui_.rollOffLevelComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), [this](auto index) {
		saveSoxrResampler(ui_.selectResamplerComboBox->currentText());
    });

	(void)QObject::connect(ui_.soxrSettingCombo, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::activated), [this](auto index) {
		auto soxr_settings = QVariant::fromValue(JsonSettings::getValue(index)).toMap();
		loadSoxrResampler(soxr_settings);
		});
}

void PreferencePage::initLang() {
	LocaleLanguage current_lang(AppSettings::getValueAsString(kAppSettingLang));

	auto current_index = 0;
	auto index = 0;
    Q_FOREACH (auto lang, LocaleLanguageManager::languageNames()) {
        QIcon ico(Q_STR(":/xamp/Resource/Flags/%1.png").arg(lang.countryIsoCode()));
		ui_.langCombo->addItem(ico, lang.nativeNameLang());
		if (current_lang.getIsoCode() == lang.getIsoCode()) {
			current_index = index;
		}
		index++;
	}

	ui_.langCombo->setCurrentIndex(current_index);

	(void)QObject::connect(ui_.langCombo, static_cast<void (QComboBox::*)(int32_t)>(&QComboBox::activated), [this](auto const& index) {		
		AppSettings::loadLanguage(ui_.langCombo->itemText(index));
		AppSettings::setValue(kAppSettingLang, ui_.langCombo->itemText(index));
		ui_.retranslateUi(this);
		saveAll();
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

	(void)QObject::connect(ui_.replayGainModeCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), [this](auto index) {
		switch (index) {
		case 0:
			AppSettings::setValue(kAppSettingReplayGainMode, static_cast<int32_t>(ReplayGainMode::RG_ALBUM_MODE));
			break;
		case 1:
			AppSettings::setValue(kAppSettingReplayGainMode, static_cast<int32_t>(ReplayGainMode::RG_TRACK_MODE));
			break;
		case 2:
			AppSettings::setValue(kAppSettingReplayGainMode, static_cast<int32_t>(ReplayGainMode::RG_NONE_MODE));
			break;
		}
		});

	ui_.replayGainModeCombo->setCurrentIndex(AppSettings::getValue(kAppSettingReplayGainMode).toInt());

	(void)QObject::connect(ui_.selectResamplerComboBox, static_cast<void (QComboBox::*)(int32_t)>(&QComboBox::activated), [this](auto const& index) {
		ui_.resamplerStackedWidget->setCurrentIndex(index);
		saveSoxrResampler(ui_.selectResamplerComboBox->currentText());
        saveAll();
		});

    (void)QObject::connect(ui_.soxrPassbandSlider, &QSlider::valueChanged, [this](auto value) {
        ui_.soxrPassbandValue->setText(QString(Q_UTF8("%0%")).arg(ui_.soxrPassbandSlider->value()));
		saveSoxrResampler(ui_.selectResamplerComboBox->currentText());
        saveAll();
    });

    (void)QObject::connect(ui_.soxrPhaseSlider, &QSlider::valueChanged, [this](auto value) {
		setPhasePercentText(ui_.soxrPhaseSlider->value());
		saveSoxrResampler(ui_.selectResamplerComboBox->currentText());
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
	JsonSettings::setValue(ui_.soxrSettingCombo->currentText(), getSoxrSettings());
	AppSettings::setValue(kAppSettingSoxrSettingName, ui_.soxrSettingCombo->currentText());
	AppSettings::setDefaultValue(kAppSettingSoxrSettingName, ui_.soxrSettingCombo->currentText());
	AppSettings::setValue(kFlacEncodingLevel, ui_.flacCompressionLevelSlider->value());

	auto index = ui_.resamplerStackedWidget->currentIndex();
	AppSettings::setValue(kAppSettingResamplerEnable, index > 0);

	JsonSettings::save();
	AppSettings::save();
}
