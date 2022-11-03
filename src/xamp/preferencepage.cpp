#include <QFileDialog>
#include <QDir>
#include <QInputDialog>
#include <QStandardItemModel>

#include <stream/dsd_times.h>

#include <widget/str_utilts.h>
#include <widget/ui_utilts.h>
#include <widget/appsettings.h>
#include <widget/localelanguage.h>
#include <widget/jsonsettings.h>
#include <widget/pixmapcache.h>
#include <widget/widget_shared.h>

#include <preferencepage.h>

void PreferencePage::updateSoxrConfigUI(const QVariantMap& soxr_settings) {
	ui_.soxrTargetSampleRateComboBox->setCurrentText(QString::number(soxr_settings[kResampleSampleRate].toInt()));
	ui_.soxrResampleQualityComboBox->setCurrentIndex(soxr_settings[kSoxrQuality].toInt());
	ui_.soxrPassbandSlider->setValue(soxr_settings[kSoxrPassBand].toInt());
	ui_.soxrPassbandValue->setText(QString(Q_TEXT("%0%")).arg(ui_.soxrPassbandSlider->value()));
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
	settings[kResampleSampleRate] = soxr_sample_rate;
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

void PreferencePage::saveR8BrainResampler() {
	QMap<QString, QVariant> settings;
	settings[kResampleSampleRate] = ui_.r8brainTargetSampleRateComboBox->currentText().toInt();
	JsonSettings::setValue(kR8Brain, settings);
}

void PreferencePage::savePcm2Dsd() {
	QMap<QString, QVariant> settings;
	settings[kPCM2DSDDsdTimes] = ui_.dsdTimeComboBox->currentIndex() + 3;
	JsonSettings::setValue(kPCM2DSD, settings);
	AppSettings::setValue(kEnablePcm2Dsd, ui_.enablePcm2DsdCheckBox->checkState() == Qt::Checked);
}

void PreferencePage::initPcm2Dsd() {
	ui_.enablePcm2DsdCheckBox->setCheckState(AppSettings::getValueAsBool(kEnablePcm2Dsd) ? Qt::Checked : Qt::Unchecked);

	auto config = JsonSettings::getValueAsMap(kPCM2DSD);
	auto dsd_times = static_cast<DsdTimes>(config[kPCM2DSDDsdTimes].toInt());
	ui_.dsdTimeComboBox->setCurrentIndex(config[kPCM2DSDDsdTimes].toInt() - 3);

	(void)QObject::connect(ui_.dsdTimeComboBox, &QComboBox::textActivated, [this](auto) {
		savePcm2Dsd();
		});

	(void)QObject::connect(ui_.enablePcm2DsdCheckBox, &QCheckBox::stateChanged, [this](auto) {
		savePcm2Dsd();
		});
}

void PreferencePage::initR8BrainResampler() {
	auto config = JsonSettings::getValueAsMap(kR8Brain);
	ui_.r8brainTargetSampleRateComboBox->setCurrentText(QString::number(config[kResampleSampleRate].toInt()));

	(void)QObject::connect(ui_.r8brainTargetSampleRateComboBox, &QComboBox::textActivated, [this](auto) {
		saveR8BrainResampler();
		});	
}

void PreferencePage::initSoxResampler() {
	auto soxr_config = JsonSettings::getValueAsMap(kSoxr);

    Q_FOREACH (const auto &soxr_setting_name, soxr_config.keys()) {
		ui_.soxrSettingCombo->addItem(soxr_setting_name);
	}
	
    const auto soxr_settings = soxr_config[AppSettings::getValueAsString(kAppSettingSoxrSettingName)].toMap();
	updateSoxrConfigUI(soxr_settings);

    ui_.enableFramelessWindowPushButton->setSwitchOn(AppSettings::getValueAsBool(kAppSettingUseFramelessWindow));
    (void)QObject::connect(ui_.enableFramelessWindowPushButton, &SwitchButton::pressed, [this]() {
        AppSettings::setValue(kAppSettingUseFramelessWindow,
                              !AppSettings::getValueAsBool(kAppSettingUseFramelessWindow));
    });

	ui_.enableBlurCoverImagePushButton->setSwitchOn(AppSettings::getValueAsBool(kEnableBlurCover));
	(void)QObject::connect(ui_.enableBlurCoverImagePushButton, &SwitchButton::pressed, [this]() {
		AppSettings::setValue(kEnableBlurCover,
			!AppSettings::getValueAsBool(kEnableBlurCover));
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
	auto str = QString(Q_TEXT("%0%")).arg(value);
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

class ActiveListWidget : public QWidget {
public:
	ActiveListWidget(PreferencePage *page, const QString &name, QStandardItemModel *model, QModelIndex index) {
		auto* layout = new QHBoxLayout(this);
		auto* namelbl = new QLabel(name);
		delete_button = new QPushButton(Q_TEXT("X"));
		delete_button->setMaximumSize(QSize(16, 16));
		layout->addWidget(namelbl, Qt::AlignLeft);
		layout->addWidget(delete_button, Qt::AlignRight);
		layout->setContentsMargins(0, 0, 0, 0);
		QObject::connect(delete_button, &QPushButton::clicked, [=] {
			page->adapter->RemoveDSPChain(name.toStdString());
			model->removeRow(index.row());
			});
	}

	QPushButton* delete_button;
};

PreferencePage::PreferencePage(QWidget *parent)
    : QFrame(parent) {
    ui_.setupUi(this);

	/*qTheme.setMenuStyle(ui_.langCombo->view()->window());
	qTheme.setMenuStyle(ui_.replayGainModeCombo->view()->window());

	qTheme.setMenuStyle(ui_.selectResamplerComboBox->view()->window());
	qTheme.setMenuStyle(ui_.soxrSettingCombo->view()->window());
	qTheme.setMenuStyle(ui_.soxrTargetSampleRateComboBox->view()->window());
	qTheme.setMenuStyle(ui_.soxrResampleQualityComboBox->view()->window());
	qTheme.setMenuStyle(ui_.rollOffLevelComboBox->view()->window());
	qTheme.setMenuStyle(ui_.r8brainTargetSampleRateComboBox->view()->window());*/

	update();

	initSoxResampler();
	initR8BrainResampler();
	initPcm2Dsd();
	initLang();

	switch (qTheme.themeColor()) {
	case ThemeColor::LIGHT_THEME:
		ui_.lightRadioButton->setChecked(true);
		break;
	case ThemeColor::DARK_THEME:
		ui_.darkRadioButton->setChecked(true);
		break;
	}

	(void)QObject::connect(ui_.lightRadioButton, &QRadioButton::clicked, [](auto checked) {
		qTheme.setThemeColor(ThemeColor::LIGHT_THEME);
		});

	(void)QObject::connect(ui_.darkRadioButton, &QRadioButton::clicked, [](auto checked) {
		qTheme.setThemeColor(ThemeColor::DARK_THEME);
		});

    ui_.preferenceTreeWidget->header()->hide();
    
    auto* settings_item = new QTreeWidgetItem(QStringList() << tr("Settings"));
    settings_item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);

    auto* dsp_manager_item = new QTreeWidgetItem(QStringList() << tr("Resampler"));
    settings_item->addChild(dsp_manager_item);

	auto* pcm2dsd_item = new QTreeWidgetItem(QStringList() << tr("PCM-DSD Covert"));
	settings_item->addChild(pcm2dsd_item);

	auto* foobar2000_dsp_item = new QTreeWidgetItem(QStringList() << tr("Foobar2000 DSP"));
	settings_item->addChild(foobar2000_dsp_item);

    ui_.preferenceTreeWidget->addTopLevelItem(settings_item);
    ui_.preferenceTreeWidget->expandAll();
	ui_.preferenceTreeWidget->setCurrentItem(settings_item);

	auto * available_dsp_model = new QStandardItemModel(this);
	adapter = MakeAlign<FoobarDspAdapter>();
	for (const auto& dsp_name : adapter->GetAvailableDSPs()) {
		available_dsp_model->appendRow(new QStandardItem(QString::fromStdString(dsp_name)));
	}	
	ui_.availableDSPListView->setModel( available_dsp_model);

	auto* active_dsp_model = new QStandardItemModel(this);
	ui_.activeDSPListView->setModel(active_dsp_model);
	ui_.activeDSPListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
	ui_.availableDSPListView->setEditTriggers(QAbstractItemView::NoEditTriggers);

	(void)QObject::connect(ui_.availableDSPListView, &QListView::doubleClicked, [active_dsp_model, available_dsp_model, this](auto index) {
		auto name = available_dsp_model->data(index).toString();
		active_dsp_model->appendRow(new QStandardItem(name));
		auto active_index = active_dsp_model->index(active_dsp_model->rowCount() - 1, 0);
		ui_.activeDSPListView->setIndexWidget(active_index, new ActiveListWidget(this, name, active_dsp_model, active_index));
		adapter->AddDSPChain(name.toStdString());
		});

	(void)QObject::connect(ui_.activeDSPListView, &QListView::doubleClicked, [active_dsp_model, available_dsp_model, this](auto index) {
		auto name = active_dsp_model->data(index).toString();
		adapter->ConfigPopup(name.toStdString(), winId());
		});

    (void)QObject::connect(ui_.preferenceTreeWidget, &QTreeWidget::itemClicked, [this](auto item, auto column) {
        const std::map<QString, int32_t> stack_page_map{
            { tr("Playback"), 0 },
            { tr("Resampler"), 1 },
			{ tr("PCM-DSD Covert"), 2 },
			{ tr("Foobar2000 DSP"), 3 },
        };

	    const auto select_type = item->text(column);
		const auto itr = stack_page_map.find(select_type);

        if (itr != stack_page_map.end()) {
            ui_.stackedWidget->setCurrentIndex((*itr).second);
        } else {
            ui_.stackedWidget->setCurrentIndex(0);
        }
    });

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
		switch (index) {
		case 1:
			saveSoxrResampler(ui_.soxrSettingCombo->currentText());
			AppSettings::setValue(kAppSettingResamplerType, kSoxr);
			break;
		case 2:
			saveR8BrainResampler();
			AppSettings::setValue(kAppSettingResamplerType, kR8Brain);
			break;
		}		
        saveAll();
		});

    (void)QObject::connect(ui_.soxrPassbandSlider, &QSlider::valueChanged, [this](auto) {
        ui_.soxrPassbandValue->setText(QString(Q_TEXT("%0%")).arg(ui_.soxrPassbandSlider->value()));
		saveSoxrResampler(ui_.soxrSettingCombo->currentText());
        saveAll();
    });

    (void)QObject::connect(ui_.soxrPhaseSlider, &QSlider::valueChanged, [this](auto) {
		setPhasePercentText(ui_.soxrPhaseSlider->value());
		saveSoxrResampler(ui_.soxrSettingCombo->currentText());
        saveAll();
    });

	(void)QObject::connect(ui_.resetAllButton, &QPushButton::clicked, [this]() {
		JsonSettings::remove(kSoxr);
		initSoxResampler();
        saveAll();
		});

    ui_.albumImageCacheSizeSpinBox->setValue(AppSettings::getValue(kAppSettingAlbumImageCacheSize).toInt());
    (void)QObject::connect(ui_.albumImageCacheSizeSpinBox, static_cast<void (QSpinBox::*)(int32_t)>(&QSpinBox::valueChanged), [](auto value) {
        qPixmapCache.setMaxSize(value);
        AppSettings::setValue(kAppSettingAlbumImageCacheSize, value);
    });
		
	setStyleSheet(Q_TEXT(R"(
			QFrame#PreferenceDialog { 
				background-color: transparent;
			}
            )"));

	ui_.stackedWidget->setStyleSheet(Q_TEXT(R"(
			QStackedWidget#stackedWidget {
				background-color: transparent;
            }
            )"));
	ui_.preferenceTreeWidget->setStyleSheet(Q_TEXT("QTreeView { background: transparent; }"));

	/*
	ui_.playbackPage->setStyleSheet(Q_TEXT("background-color: transparent;"));
	ui_.noneResamplerPage->setStyleSheet(Q_TEXT("background-color: transparent;"));
	ui_.soxrResamplerPage->setStyleSheet(Q_TEXT("background-color: transparent;"));
	ui_.r8brainResamplerPage->setStyleSheet(Q_TEXT("background-color: transparent;"));
	ui_.pcm2dsdPage->setStyleSheet(Q_TEXT("background-color: transparent;"));
	ui_.dspManagerPage->setStyleSheet(Q_TEXT("background-color: transparent;"));*/
}

void PreferencePage::update() {
	const auto enable_resampler = AppSettings::getValueAsBool(kAppSettingResamplerEnable);
	if (!enable_resampler) {
		ui_.resamplerStackedWidget->setCurrentIndex(0);
		ui_.selectResamplerComboBox->setCurrentIndex(0);
	}
	else {
		auto resampler_type = AppSettings::getValueAsString(kAppSettingResamplerType);
		if (resampler_type == kSoxr || resampler_type.isEmpty()) {
			ui_.resamplerStackedWidget->setCurrentIndex(1);
			ui_.selectResamplerComboBox->setCurrentIndex(1);
		}
		else if (resampler_type == kR8Brain) {
			ui_.resamplerStackedWidget->setCurrentIndex(2);
			ui_.selectResamplerComboBox->setCurrentIndex(2);
		}
	}
}

void PreferencePage::saveAll() {
	saveSoxrResampler(ui_.soxrSettingCombo->currentText());
	saveR8BrainResampler();

	auto index = ui_.resamplerStackedWidget->currentIndex();
	AppSettings::setValue(kAppSettingResamplerEnable, index > 0);

	JsonSettings::save();
	AppSettings::save();
}
