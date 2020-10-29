#include <QSlider>
#include <QTreeWidget>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QDir>
#include <QInputDialog>

#include <base/Uuid.h>

#include <widget/str_utilts.h>
#include <widget/appsettings.h>
#include <widget/localelanguage.h>
#include <widget/jsonsettings.h>

#include <preferencedialog.h>

void PreferenceDialog::loadSoxrResampler(const QVariantMap& soxr_settings) {
	ui_.soxrTargetSampleRateComboBox->setCurrentText(QString::number(soxr_settings[kSoxrResampleSampleRate].toInt()));
	ui_.soxrResampleQualityComboBox->setCurrentIndex(soxr_settings[kSoxrQuality].toInt());
	ui_.soxrPhaseComboBox->setCurrentIndex(soxr_settings[kSoxrPhase].toInt());
	ui_.soxrPassbandSlider->setValue(soxr_settings[kSoxrPassBand].toInt());
	ui_.soxrPassbandValue->setText(QString(Q_UTF8("%0%")).arg(ui_.soxrPassbandSlider->value()));

	if (soxr_settings[kSoxrEnableSteepFilter].toBool()) {
		ui_.soxrAllowAliasingCheckBox->setChecked(true);
	}
	else {
		ui_.soxrAllowAliasingCheckBox->setChecked(false);
	}
}

void PreferenceDialog::saveSoxrResampler(const QString &name) {
    const auto soxr_sample_rate = ui_.soxrTargetSampleRateComboBox->currentText().toInt();
    const auto soxr_quility = ui_.soxrResampleQualityComboBox->currentIndex();
    const auto soxr_phase = ui_.soxrPhaseComboBox->currentIndex();
    const auto soxr_pass_band = ui_.soxrPassbandSlider->value();
    const auto soxr_enable_steep_filter = ui_.soxrAllowAliasingCheckBox->checkState() == Qt::Checked;

    QMap<QString, QVariant> settings;
    settings[kSoxrResampleSampleRate] = soxr_sample_rate;
    settings[kSoxrEnableSteepFilter] = soxr_enable_steep_filter;
    settings[kSoxrQuality] = soxr_quility;
    settings[kSoxrPhase] = soxr_phase;
    settings[kSoxrPassBand] = soxr_pass_band;

    JsonSettings::setValue(name, settings);
}

void PreferenceDialog::initSoxResampler() {
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

void PreferenceDialog::initLang() {
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

PreferenceDialog::PreferenceDialog(QWidget *parent)
    : QDialog(parent) {
    ui_.setupUi(this);
    setWindowTitle(tr("Preferences"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    ui_.preferenceTreeWidget->header()->hide();
	setFixedSize(QSize(700, 640));

    auto playback_item = new QTreeWidgetItem(QStringList() << tr("Playback"));
    playback_item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);

    const auto dsp_manager_item = new QTreeWidgetItem(QStringList() << tr("Resampler"));
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

    (void)QObject::connect(ui_.soxrPassbandSlider, &QSlider::sliderMoved, [this](auto) {
        ui_.soxrPassbandValue->setText(QString(Q_UTF8("%0%")).arg(ui_.soxrPassbandSlider->value()));
    });

    music_file_path_ = AppSettings::getValue(kAppSettingMusicFilePath).toString();
    ui_.musicFilePath->setText(music_file_path_);

	(void)QObject::connect(ui_.setPathButton, &QPushButton::clicked, [this]() {
        music_file_path_ = QFileDialog::getExistingDirectory(
			this,
			tr("Select a directory"),
			QDir::currentPath());
        AppSettings::setValue(kAppSettingMusicFilePath, music_file_path_);
        ui_.musicFilePath->setText(music_file_path_);
		});

	(void)QObject::connect(ui_.resetAllButton, &QPushButton::clicked, [this]() {
		initSoxResampler();
        AppSettings::setOrDefaultConfig();
		});

    (void)QObject::connect(ui_.buttonBox, &QDialogButtonBox::accepted, [this]() {
		const auto soxr_sample_rate = ui_.soxrTargetSampleRateComboBox->currentText().toInt();
		const auto soxr_quility = ui_.soxrResampleQualityComboBox->currentIndex();
		const auto soxr_phase = ui_.soxrPhaseComboBox->currentIndex();
		const auto soxr_pass_band = ui_.soxrPassbandSlider->value();
		const auto soxr_enable_steep_filter = ui_.soxrAllowAliasingCheckBox->checkState() == Qt::Checked;

		QMap<QString, QVariant> settings;
		settings[kSoxrResampleSampleRate] = soxr_sample_rate;
		settings[kSoxrEnableSteepFilter] = soxr_enable_steep_filter;
		settings[kSoxrQuality] = soxr_quility;
		settings[kSoxrPhase] = soxr_phase;
		settings[kSoxrPassBand] = soxr_pass_band;

		JsonSettings::setValue(ui_.soxrSettingCombo->currentText(), settings);
		AppSettings::setValue(kAppSettingSoxrSettingName, ui_.soxrSettingCombo->currentText());
		AppSettings::setDefaultValue(kAppSettingSoxrSettingName, ui_.soxrSettingCombo->currentText());

		auto index = ui_.resamplerStackedWidget->currentIndex();
		AppSettings::setValue(kAppSettingResamplerEnable, index > 0);
		});	

	initSoxResampler();
	initLang();
}
