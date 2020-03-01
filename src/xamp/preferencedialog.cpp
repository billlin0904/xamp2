#include <QSlider>
#include <QTreeWidget>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QDir>

#include <base/id.h>

#include <widget/str_utilts.h>
#include <widget/appsettings.h>
#include <preferencedialog.h>

void PreferenceDialog::initSoxResampler() {
	ui_.soxrTargetSampleRateComboBox->setCurrentText(QString::number(AppSettings::getValue(APP_SETTING_SOXR_RESAMPLE_SAMPLRATE).toInt()));
	ui_.soxrResampleQualityComboBox->setCurrentIndex(AppSettings::getValue(APP_SETTING_SOXR_QUALITY).toInt());
	ui_.soxrPhaseComboBox->setCurrentIndex(AppSettings::getValue(APP_SETTING_SOXR_PHASE).toInt());
	ui_.soxrPassbandSlider->setValue(AppSettings::getValue(APP_SETTING_SOXR_PASS_BAND).toInt());
	ui_.soxrPassbandValue->setText(QString(Q_UTF8("%0%")).arg(ui_.soxrPassbandSlider->value()));

	auto enable_soxr = AppSettings::getValue(APP_SETTING_SOXR_ENABLE).toBool();
	if (enable_soxr) {
		ui_.resamplerStackedWidget->setCurrentIndex(1);
		ui_.selectResamplerComboBox->setCurrentIndex(1);
	}
	else {
		ui_.resamplerStackedWidget->setCurrentIndex(0);
		ui_.selectResamplerComboBox->setCurrentIndex(0);
	}

	auto enable = AppSettings::getValue(APP_SETTING_SOXR_ENABLE_STEEP_FILTER).toBool();
	if (enable) {
		ui_.soxrAllowAliasingCheckBox->setChecked(true);
	}
	else {
		ui_.soxrAllowAliasingCheckBox->setChecked(false);
	}
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

    const auto dsp_manager_item = new QTreeWidgetItem(QStringList() << tr("Soxr"));
    playback_item->addChild(dsp_manager_item);

    ui_.preferenceTreeWidget->addTopLevelItem(playback_item);
    ui_.preferenceTreeWidget->expandAll();

    (void) QObject::connect(ui_.preferenceTreeWidget, &QTreeWidget::itemClicked, [this](auto item, auto column) {
        const std::map<QString, int32_t> stack_page_map{
            { tr("Playback"), 0 },
            { tr("Soxr"), 1 },
        };

	    const auto select_type = item->text(column);
		const auto itr = stack_page_map.find(select_type);

        if (itr != stack_page_map.end()) {
            ui_.stackedWidget->setCurrentIndex((*itr).second);
        } else {
            ui_.stackedWidget->setCurrentIndex(0);
        }
    });

	(void)QObject::connect(ui_.selectResamplerComboBox, static_cast<void (QComboBox::*)(int32_t)>(&QComboBox::activated), [this](auto const& index) {
		ui_.resamplerStackedWidget->setCurrentIndex(index);
		});

	(void)QObject::connect(ui_.soxrPassbandSlider, &QSlider::sliderReleased, [this]() {
		ui_.soxrPassbandValue->setText(QString(Q_UTF8("%0%")).arg(ui_.soxrPassbandSlider->value()));
		});

	ui_.musicFilePath->setText(AppSettings::getValue(APP_SETTING_MUSIC_FILE_PATH).toString());

	(void)QObject::connect(ui_.setPathButton, &QPushButton::clicked, [this]() {
		musicFilePath = QFileDialog::getExistingDirectory(
			this,
			tr("Select a directory"),
			QDir::currentPath());
		AppSettings::setValue(APP_SETTING_MUSIC_FILE_PATH, musicFilePath);
		ui_.musicFilePath->setText(musicFilePath);
		});

	(void)QObject::connect(ui_.resetAllButton, &QPushButton::clicked, [this]() {
		AppSettings::setValue(APP_SETTING_SOXR_ENABLE, false);
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

		AppSettings::setValue(APP_SETTING_SOXR_ENABLE, ui_.resamplerStackedWidget->currentIndex() > 0);
		AppSettings::setValue(APP_SETTING_SOXR_RESAMPLE_SAMPLRATE, soxr_sample_rate);
		AppSettings::setValue(APP_SETTING_SOXR_ENABLE_STEEP_FILTER, soxr_enable_steep_filter);
		AppSettings::setValue(APP_SETTING_SOXR_QUALITY, soxr_quility);
		AppSettings::setValue(APP_SETTING_SOXR_PHASE, soxr_phase);
		AppSettings::setValue(APP_SETTING_SOXR_PASS_BAND, soxr_pass_band);
		});	

	initSoxResampler();
}
