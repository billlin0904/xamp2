#include <QCheckBox>
#include <widget/preferencepage.h>
#include <ui_preferencedialog.h>
#include <QInputDialog>
#include <QStandardItemModel>

#include <widget/str_utilts.h>
#include <widget/ui_utilts.h>
#include <widget/appsettings.h>
#include <widget/localelanguage.h>
#include <widget/jsonsettings.h>
#include <widget/imagecache.h>
#include <widget/widget_shared.h>

#include <thememanager.h>

class ActiveListWidget : public QWidget {
public:
	ActiveListWidget(PreferencePage* page, const QString& name, QStandardItemModel* model, QModelIndex index) {
		auto* layout = new QHBoxLayout(this);
		auto* namelbl = new QLabel(name);
		delete_button = new QPushButton(qTEXT("X"));
		delete_button->setMaximumSize(QSize(16, 16));
		layout->addWidget(namelbl, Qt::AlignLeft);
		layout->addWidget(delete_button, Qt::AlignRight);
		layout->setContentsMargins(0, 0, 0, 0);
		(void)QObject::connect(delete_button, &QPushButton::clicked, [=] {
			model->removeRow(index.row());
			});
	}

	QPushButton* delete_button;
};

void PreferencePage::updateSoxrConfigUi(const QVariantMap& soxr_settings) {
	ui_->soxrTargetSampleRateComboBox->setCurrentText(QString::number(soxr_settings[kResampleSampleRate].toInt()));
	ui_->soxrResampleQualityComboBox->setCurrentIndex(soxr_settings[kSoxrQuality].toInt());
	ui_->soxrPassbandSlider->setValue(soxr_settings[kSoxrPassBand].toInt());
	ui_->soxrPassbandValue->setText(QString(qTEXT("%0%")).arg(ui_->soxrPassbandSlider->value()));
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
	// todo: kSoxrStopBand always set 100?
	settings[kSoxrStopBand] = 100;

	return settings;
}

void PreferencePage::saveSoxrResampler(const QString &name) const {
	auto soxr_config = JsonSettings::valueAsMap(kSoxr);

	soxr_config[name] = currentSoxrSettings();
	JsonSettings::setValue(kSoxr, soxr_config);
	qAppSettings.setValue(kAppSettingSoxrSettingName, name);

	if (!soxr_config.contains(name)) {
		ui_->soxrSettingCombo->addItem(name);
	}
}

void PreferencePage::saveSrcResampler() {
	QMap<QString, QVariant> settings;
	settings[kResampleSampleRate] = ui_->srcTargetSampleRateComboBox->currentText().toInt();
	JsonSettings::setValue(kSrc, settings);
}

void PreferencePage::saveR8BrainResampler() {
	QMap<QString, QVariant> settings;
	settings[kResampleSampleRate] = ui_->r8brainTargetSampleRateComboBox->currentText().toInt();
	JsonSettings::setValue(kR8Brain, settings);
}

void PreferencePage::initSrcResampler() {
	auto config = JsonSettings::valueAsMap(kSrc);
	ui_->srcTargetSampleRateComboBox->setCurrentText(QString::number(config[kResampleSampleRate].toInt()));

	(void)QObject::connect(ui_->srcTargetSampleRateComboBox, &QComboBox::textActivated, [this](auto) {
		saveSrcResampler();
		});
}

void PreferencePage::initR8BrainResampler() {
	auto config = JsonSettings::valueAsMap(kR8Brain);
	ui_->r8brainTargetSampleRateComboBox->setCurrentText(QString::number(config[kResampleSampleRate].toInt()));

	(void)QObject::connect(ui_->r8brainTargetSampleRateComboBox, &QComboBox::textActivated, [this](auto) {
		saveR8BrainResampler();
		});	
}

void PreferencePage::initSoxResampler() {
	auto soxr_config = JsonSettings::valueAsMap(kSoxr);

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
        JsonSettings::remove(name);
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
		const auto soxr_settings = JsonSettings::valueAsMap(kSoxr);
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

	(void)QObject::connect(ui_->langCombo, static_cast<void (QComboBox::*)(int32_t)>(&QComboBox::activated), 
		[this](auto const& index) {
		setLanguage(index);
		});
}

void PreferencePage::setPhasePercentText(int32_t value) {
	auto str = QString(qTEXT("%0%")).arg(value);
	if (value == 0) {
		str += qTR(" (minimum)");
	}
	else if (value < 50 && value != 0) {
		str += qTR(" (intermediate)");
	}
	else if (value == 50) {
		str += qTR(" (linear)");
	}
	ui_->soxrPhaseValue->setText(str);
}

PreferencePage::PreferencePage(QWidget *parent)
    : QFrame(parent) {
	ui_ = new Ui::PreferenceDialog();
    ui_->setupUi(this);

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
		emit qTheme.currentThemeChanged(ThemeColor::LIGHT_THEME);
		});

	(void)QObject::connect(ui_->darkRadioButton, &QRadioButton::clicked, [](auto checked) {
		emit qTheme.currentThemeChanged(ThemeColor::DARK_THEME);
		});

    ui_->preferenceTreeWidget->header()->hide();
    
    auto* settings_item = new QTreeWidgetItem(QStringList() << qTR("Playback"));
    settings_item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);

    auto* dsp_manager_item = new QTreeWidgetItem(QStringList() << qTR("Resampler"));
    settings_item->addChild(dsp_manager_item);

    ui_->preferenceTreeWidget->addTopLevelItem(settings_item);
    ui_->preferenceTreeWidget->expandAll();
	ui_->preferenceTreeWidget->setCurrentItem(settings_item);

    (void)QObject::connect(ui_->preferenceTreeWidget, &QTreeWidget::itemClicked, [this](auto item, auto column) {
        const OrderedMap<QString, int32_t> stack_page_map{
            { qTR("Playback"), 0 },
            { qTR("Resampler"), 1 },
        };

	    const auto select_type = item->text(column);
		const auto itr = stack_page_map.find(select_type);

        if (itr != stack_page_map.end()) {
            ui_->stackedWidget->setCurrentIndex((*itr).second);
        } else {
            ui_->stackedWidget->setCurrentIndex(0);
        }
    });

    (void)QObject::connect(ui_->replayGainModeCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), [](auto index) {
		switch (index) {
		case 0:
			qAppSettings.setEnumValue(kAppSettingReplayGainMode, ReplayGainMode::RG_ALBUM_MODE);
			qAppSettings.setValue(kAppSettingEnableReplayGain, true);
			break;
		case 1:
			qAppSettings.setEnumValue(kAppSettingReplayGainMode, ReplayGainMode::RG_TRACK_MODE);
			qAppSettings.setValue(kAppSettingEnableReplayGain, true);
			break;
		case 2:
			qAppSettings.setEnumValue(kAppSettingReplayGainMode, ReplayGainMode::RG_NONE_MODE);
			qAppSettings.setValue<bool>(kAppSettingEnableReplayGain, false);			
			break;
		}
		});

	ui_->replayGainModeCombo->setCurrentIndex(qAppSettings.valueAs(kAppSettingReplayGainMode).toInt());

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

    (void)QObject::connect(ui_->soxrPassbandSlider, &QSlider::valueChanged, [this](auto) {
        ui_->soxrPassbandValue->setText(QString(qTEXT("%0%")).arg(ui_->soxrPassbandSlider->value()));
		saveSoxrResampler(ui_->soxrSettingCombo->currentText());
        saveAll();
    });

    (void)QObject::connect(ui_->soxrPhaseSlider, &QSlider::valueChanged, [this](auto) {
		setPhasePercentText(ui_->soxrPhaseSlider->value());
		saveSoxrResampler(ui_->soxrSettingCombo->currentText());
        saveAll();
    });

	(void)QObject::connect(ui_->newSoxrSettingBtn, &QRadioButton::clicked, [this](auto checked) {
		const auto setting_name = QInputDialog::getText(this, qTR("New soxr setting"),
		qTR("Setting name:"),
		QLineEdit::Normal,
		ui_->soxrSettingCombo->currentText());
		if (setting_name.isEmpty()) {
			return;
		}
		auto soxr_config = JsonSettings::valueAsMap(kSoxr);
		if (soxr_config.contains(setting_name)) {
			return;
		}
		saveSoxrResampler(setting_name);
		});

	(void)QObject::connect(ui_->resetAllButton, &QPushButton::clicked, [this]() {
		JsonSettings::remove(kSoxr);
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
		w->setStyleSheet(qTEXT("background: transparent;"));
	}

	Q_FOREACH(auto* w, soxr_page_widgets) {
		w->setStyleSheet(qTEXT("background: transparent;"));
	}

	Q_FOREACH(auto* w, r8brain_page_widgets) {
		w->setStyleSheet(qTEXT("background: transparent;"));
	}

	ui_->soxrPhaseSlider->setRange(1, 100);
	ui_->soxrPassbandSlider->setRange(1, 100);

	qTheme.setSliderTheme(ui_->soxrPhaseSlider, true);
	qTheme.setSliderTheme(ui_->soxrPassbandSlider, true);

	ui_->stackedWidget->setCurrentIndex(0);
	setFixedSize(950, 700);		
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

	JsonSettings::save();
	qAppSettings.save();
}
