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
#include "thememanager.h"
#include <preferencepage.h>

void PreferencePage::loadSoxrResampler(const QVariantMap& soxr_settings) {
	ui_.soxrTargetSampleRateComboBox->setCurrentText(QString::number(soxr_settings[kSoxrResampleSampleRate].toInt()));
	ui_.soxrResampleQualityComboBox->setCurrentIndex(soxr_settings[kSoxrQuality].toInt());
	ui_.soxrPassbandSlider->setValue(soxr_settings[kSoxrPassBand].toInt());
	ui_.soxrPassbandValue->setText(QString(Q_UTF8("%0%")).arg(ui_.soxrPassbandSlider->value()));
    ui_.soxrPhaseSlider->setValue(soxr_settings[kSoxrPhase].toInt());
    ui_.soxrPhaseValue->setText(QString(Q_UTF8("%0%")).arg(ui_.soxrPhaseSlider->value()));

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
	const auto soxr_enable_steep_filter = ui_.enableSteepFilterBox->checkState() == Qt::Checked;

	QMap<QString, QVariant> settings;
	settings[kSoxrResampleSampleRate] = soxr_sample_rate;
	settings[kSoxrEnableSteepFilter] = soxr_enable_steep_filter;
	settings[kSoxrQuality] = soxr_quility;
	if (soxr_passband_ > 0) {
		settings[kSoxrPassBand] = soxr_passband_;
	} else {
		settings[kSoxrPassBand] = soxr_pass_band;
    }

    if (soxr_phase_ > 0) {
        settings[kSoxrPhase] = soxr_phase_;
    } else {
        settings[kSoxrPhase] = soxr_phase;
    }
	return settings;
}

void PreferencePage::saveSoxrResampler(const QString &name) {
    JsonSettings::setValue(name, getSoxrSettings());
}

void PreferencePage::initSoxResampler() {
    for (const auto &soxr_setting_name : JsonSettings::keys()) {
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

	ui_.enableFramelessWindowCheckBox->setCheckState(
		AppSettings::getValueAsBool(kAppSettingUseFramelessWindow) ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);

	(void)QObject::connect(ui_.enableFramelessWindowCheckBox, &QCheckBox::stateChanged, [this](auto state) {
		AppSettings::setValue(kAppSettingUseFramelessWindow,
			static_cast<Qt::CheckState>(state) == Qt::CheckState::Checked);
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
		});

	(void)QObject::connect(ui_.deleteSoxrSettingBtn, &QPushButton::pressed, [this]() {
		auto name = ui_.soxrSettingCombo->currentText();
		if (name == kSoxrDefaultSettingName) {
			return;
		}
        JsonSettings::remove(name);
        ui_.soxrSettingCombo->removeItem(ui_.soxrSettingCombo->currentIndex());
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
	for (auto lang : LocaleLanguageManager::languageNames()) {
        QIcon ico(Q_STR(":/xamp/Resource/Flags/%1.png").arg(lang.countryIsoCode()));
		ui_.langCombo->addItem(ico, lang.getIsoCode());
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
		});
}

PreferencePage::PreferencePage(QWidget *parent)
    : QFrame(parent)
    , soxr_passband_(0)
    , soxr_phase_(0) {
    ui_.setupUi(this);
	setStyleSheet(Q_UTF8("background-color: transparent"));

    ui_.preferenceTreeWidget->header()->hide();
	ui_.preferenceTreeWidget->setStyleSheet(Q_UTF8("background-color: transparent"));

    auto* playback_item = new QTreeWidgetItem(QStringList() << tr("Playback"));
    playback_item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);

    auto* dsp_manager_item = new QTreeWidgetItem(QStringList() << tr("Resampler"));
    playback_item->addChild(dsp_manager_item);

    ui_.preferenceTreeWidget->addTopLevelItem(playback_item);
    ui_.preferenceTreeWidget->expandAll();

    (void) QObject::connect(ui_.preferenceTreeWidget, &QTreeWidget::itemClicked, [this](auto item, auto column) {
        const std::map<QString, int32_t> stack_page_map{
            { tr("Playback"), 0 },
            { tr("Resampler"), 1 },
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

	(void)QObject::connect(ui_.selectResamplerComboBox, static_cast<void (QComboBox::*)(int32_t)>(&QComboBox::activated), [this](auto const& index) {
		ui_.resamplerStackedWidget->setCurrentIndex(index);
		});

    (void)QObject::connect(ui_.soxrPassbandSlider, &QSlider::valueChanged, [this](auto value) {
        soxr_passband_ = value;
        ui_.soxrPassbandValue->setText(QString(Q_UTF8("%0%")).arg(soxr_passband_));
    });

    (void)QObject::connect(ui_.soxrPhaseSlider, &QSlider::valueChanged, [this](auto value) {
        soxr_phase_ = value;
        auto str = QString(Q_UTF8("%0%")).arg(soxr_phase_);
        if (soxr_phase_ == 0) {
            str += tr(" (minimum)");
        } else if (soxr_phase_ == 50) {
            str += tr(" (linear)");
        }
        ui_.soxrPhaseValue->setText(str);
    });

	(void)QObject::connect(ui_.resetAllButton, &QPushButton::clicked, [this]() {
		initSoxResampler();
		});

    (void)QObject::connect(ui_.buttonBox, &QDialogButtonBox::accepted, [this]() {
		JsonSettings::setValue(ui_.soxrSettingCombo->currentText(), getSoxrSettings());
		AppSettings::setValue(kAppSettingSoxrSettingName, ui_.soxrSettingCombo->currentText());
		AppSettings::setDefaultValue(kAppSettingSoxrSettingName, ui_.soxrSettingCombo->currentText());

		auto index = ui_.resamplerStackedWidget->currentIndex();
		AppSettings::setValue(kAppSettingResamplerEnable, index > 0);

		JsonSettings::save();
		AppSettings::save();
		});

	ui_.podcastCachePathLineEdit->setText(AppSettings::getValue(kAppSettingPodcastCachePath).toString());
	(void)QObject::connect(ui_.setPodcastCacheButton, &QPushButton::clicked, [this]() {
		const auto dir_name = QFileDialog::getExistingDirectory(this,
			tr("Select a directory"),
			AppSettings::getMyMusicFolderPath(),
			QFileDialog::DontUseNativeDialog | QFileDialog::ShowDirsOnly);
		if (dir_name.isEmpty()) {
			return;
		}
		PodcastCache.SetTempPath(dir_name.toStdWString());
		AppSettings::setValue(kAppSettingPodcastCachePath, dir_name);
		ui_.podcastCachePathLineEdit->setText(dir_name);
		});

	initSoxResampler();
	initLang();
}