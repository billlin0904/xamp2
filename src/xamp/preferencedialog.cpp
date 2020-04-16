#include <QSlider>
#include <QTreeWidget>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QDir>

#include <base/id.h>

#include <widget/str_utilts.h>
#include <widget/appsettings.h>
#include <widget/localelanguage.h>
#include <preferencedialog.h>

void PreferenceDialog::initSoxResampler() {
	ui_.soxrTargetSampleRateComboBox->setCurrentText(QString::number(AppSettings::getValue(APP_SETTING_SOXR_RESAMPLE_SAMPLRATE).toInt()));
	ui_.soxrResampleQualityComboBox->setCurrentIndex(AppSettings::getValue(APP_SETTING_SOXR_QUALITY).toInt());
	ui_.soxrPhaseComboBox->setCurrentIndex(AppSettings::getValue(APP_SETTING_SOXR_PHASE).toInt());
	ui_.soxrPassbandSlider->setValue(AppSettings::getValue(APP_SETTING_SOXR_PASS_BAND).toInt());
	ui_.soxrPassbandValue->setText(QString(Q_UTF8("%0%")).arg(ui_.soxrPassbandSlider->value()));

	auto enable_resampler = AppSettings::getValue(APP_SETTING_RESAMPLER_ENABLE).toBool();
	if (!enable_resampler) {
		ui_.resamplerStackedWidget->setCurrentIndex(0);
		ui_.selectResamplerComboBox->setCurrentIndex(0);
	}
	else {
		ui_.resamplerStackedWidget->setCurrentIndex(1);
		ui_.selectResamplerComboBox->setCurrentIndex(1);
	}

	auto enable = AppSettings::getValue(APP_SETTING_SOXR_ENABLE_STEEP_FILTER).toBool();
	if (enable) {
		ui_.soxrAllowAliasingCheckBox->setChecked(true);
	}
	else {
		ui_.soxrAllowAliasingCheckBox->setChecked(false);
	}
}

void PreferenceDialog::initLang() {
	LocaleLanguage defaultLang;

	auto current_index = 0;
	auto index = 0;
	for (auto lang : LocaleLanguageManager::languageNames()) {
		QIcon ico(QString(Q_UTF8(":/xamp/Resource/Flags/%1.png")).arg(lang.countryIsoCode()));
		ui_.langCombo->addItem(ico, lang.getIsoCode());
		if (defaultLang.getIsoCode() == lang.getIsoCode()) {
			current_index = index;
		}
		index++;
	}

	ui_.langCombo->setCurrentIndex(current_index);

	(void)QObject::connect(ui_.langCombo, static_cast<void (QComboBox::*)(int32_t)>(&QComboBox::activated), [this](auto const& index) {
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

#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
    ui_.selectResamplerComboBox->removeItem(2);
#endif

	(void)QObject::connect(ui_.selectResamplerComboBox, static_cast<void (QComboBox::*)(int32_t)>(&QComboBox::activated), [this](auto const& index) {
		ui_.resamplerStackedWidget->setCurrentIndex(index);
		});

	(void)QObject::connect(ui_.soxrPassbandSlider, &QSlider::sliderReleased, [this]() {
		ui_.soxrPassbandValue->setText(QString(Q_UTF8("%0%")).arg(ui_.soxrPassbandSlider->value()));
		});

	musicFilePath = AppSettings::getValue(APP_SETTING_MUSIC_FILE_PATH).toString();
	ui_.musicFilePath->setText(musicFilePath);

	(void)QObject::connect(ui_.setPathButton, &QPushButton::clicked, [this]() {
		musicFilePath = QFileDialog::getExistingDirectory(
			this,
			tr("Select a directory"),
			QDir::currentPath());
		AppSettings::setValue(APP_SETTING_MUSIC_FILE_PATH, musicFilePath);
		ui_.musicFilePath->setText(musicFilePath);
		});

	(void)QObject::connect(ui_.resetAllButton, &QPushButton::clicked, [this]() {
		AppSettings::setValue(APP_SETTING_RESAMPLER_ENABLE, false);
		AppSettings::setValue(APP_SETTING_SOXR_RESAMPLE_SAMPLRATE, 44100);
		AppSettings::setValue(APP_SETTING_SOXR_ENABLE_STEEP_FILTER, false);
		AppSettings::setValue(APP_SETTING_SOXR_QUALITY, 3);
		AppSettings::setValue(APP_SETTING_SOXR_PHASE, 0);
		AppSettings::setValue(APP_SETTING_SOXR_PASS_BAND, 99);
		initSoxResampler();
		});

	(void) QObject::connect(ui_.buttonBox, &QDialogButtonBox::accepted, [this]() {
		const auto soxr_sample_rate = ui_.soxrTargetSampleRateComboBox->currentText().toInt();
		const auto soxr_quility = ui_.soxrResampleQualityComboBox->currentIndex();
		const auto soxr_phase = ui_.soxrPhaseComboBox->currentIndex();
		const auto soxr_pass_band = ui_.soxrPassbandSlider->value();
		const auto soxr_enable_steep_filter = ui_.soxrAllowAliasingCheckBox->checkState() == Qt::Checked;

		auto index = ui_.resamplerStackedWidget->currentIndex();
		AppSettings::setValue(APP_SETTING_RESAMPLER_ENABLE, index > 0);
		AppSettings::setValue(APP_SETTING_SOXR_RESAMPLE_SAMPLRATE, soxr_sample_rate);
		AppSettings::setValue(APP_SETTING_SOXR_ENABLE_STEEP_FILTER, soxr_enable_steep_filter);
		AppSettings::setValue(APP_SETTING_SOXR_QUALITY, soxr_quility);
		AppSettings::setValue(APP_SETTING_SOXR_PHASE, soxr_phase);
		AppSettings::setValue(APP_SETTING_SOXR_PASS_BAND, soxr_pass_band);
		});	

	initSoxResampler();
	initLang();
}
