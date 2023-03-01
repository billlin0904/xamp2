#include <QFileDialog>
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

#include <thememanager.h>
#include <preferencepage.h>

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
		QObject::connect(delete_button, &QPushButton::clicked, [=] {
			model->removeRow(index.row());
			});
	}

	QPushButton* delete_button;
};

void PreferencePage::UpdateSoxrConfigUi(const QVariantMap& soxr_settings) {
	ui_.soxrTargetSampleRateComboBox->setCurrentText(QString::number(soxr_settings[kResampleSampleRate].toInt()));
	ui_.soxrResampleQualityComboBox->setCurrentIndex(soxr_settings[kSoxrQuality].toInt());
	ui_.soxrPassbandSlider->setValue(soxr_settings[kSoxrPassBand].toInt());
	ui_.soxrPassbandValue->setText(QString(qTEXT("%0%")).arg(ui_.soxrPassbandSlider->value()));
    ui_.soxrPhaseSlider->setValue(soxr_settings[kSoxrPhase].toInt());
	SetPhasePercentText(ui_.soxrPhaseSlider->value());
    ui_.rollOffLevelComboBox->setCurrentIndex(soxr_settings[kSoxrRollOffLevel].toInt());
}

QMap<QString, QVariant> PreferencePage::CurrentSoxrSettings() const {
	const auto soxr_sample_rate = ui_.soxrTargetSampleRateComboBox->currentText().toInt();
	const auto soxr_quility = ui_.soxrResampleQualityComboBox->currentIndex();
	const auto soxr_pass_band = ui_.soxrPassbandSlider->value();
	const auto soxr_phase = ui_.soxrPhaseSlider->value();
    const auto soxr_rolloff = ui_.rollOffLevelComboBox->currentIndex();
	const auto soxr_enable_steep_filter = false;

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

void PreferencePage::SaveSoxrResampler(const QString &name) const {
	QMap<QString, QVariant> soxr_settings;
	soxr_settings[name] = CurrentSoxrSettings();

	auto soxr_config= JsonSettings::ValueAsMap(kSoxr);

	JsonSettings::SetValue(kSoxr, soxr_settings);
    AppSettings::SetValue(kAppSettingSoxrSettingName, name);

	if (!soxr_config.contains(name)) {
		ui_.soxrSettingCombo->addItem(name);
	}
}

void PreferencePage::SaveR8BrainResampler() {
	QMap<QString, QVariant> settings;
	settings[kResampleSampleRate] = ui_.r8brainTargetSampleRateComboBox->currentText().toInt();
	JsonSettings::SetValue(kR8Brain, settings);
}

void PreferencePage::SavePcm2Dsd() {
	QMap<QString, QVariant> settings;
	settings[kPCM2DSDDsdTimes] = ui_.dsdTimeComboBox->currentIndex() + 3;
	JsonSettings::SetValue(kPCM2DSD, settings);
	AppSettings::SetValue(kEnablePcm2Dsd, ui_.enablePcm2DsdCheckBox->checkState() == Qt::Checked);
}

void PreferencePage::InitPcm2Dsd() {
	ui_.enablePcm2DsdCheckBox->setCheckState(AppSettings::ValueAsBool(kEnablePcm2Dsd) ? Qt::Checked : Qt::Unchecked);

	auto config = JsonSettings::ValueAsMap(kPCM2DSD);
	auto dsd_times = static_cast<DsdTimes>(config[kPCM2DSDDsdTimes].toInt());
	ui_.dsdTimeComboBox->setCurrentIndex(config[kPCM2DSDDsdTimes].toInt() - 3);

	(void)QObject::connect(ui_.dsdTimeComboBox, &QComboBox::textActivated, [this](auto) {
		SavePcm2Dsd();
		});

	(void)QObject::connect(ui_.enablePcm2DsdCheckBox, &QCheckBox::stateChanged, [this](auto) {
		SavePcm2Dsd();
		});
}

void PreferencePage::InitR8BrainResampler() {
	auto config = JsonSettings::ValueAsMap(kR8Brain);
	ui_.r8brainTargetSampleRateComboBox->setCurrentText(QString::number(config[kResampleSampleRate].toInt()));

	(void)QObject::connect(ui_.r8brainTargetSampleRateComboBox, &QComboBox::textActivated, [this](auto) {
		SaveR8BrainResampler();
		});	
}

void PreferencePage::InitSoxResampler() {
	auto soxr_config = JsonSettings::ValueAsMap(kSoxr);

    Q_FOREACH (const auto &soxr_setting_name, soxr_config.keys()) {
		ui_.soxrSettingCombo->addItem(soxr_setting_name);
	}
	
    const auto soxr_settings = soxr_config[AppSettings::ValueAsString(kAppSettingSoxrSettingName)].toMap();
	UpdateSoxrConfigUi(soxr_settings);

    ui_.enableFramelessWindowPushButton->setSwitchOn(AppSettings::ValueAsBool(kAppSettingUseFramelessWindow));
    (void)QObject::connect(ui_.enableFramelessWindowPushButton, &SwitchButton::pressed, [this]() {
        AppSettings::SetValue(kAppSettingUseFramelessWindow,
                              !AppSettings::ValueAsBool(kAppSettingUseFramelessWindow));
    });

	ui_.enableBlurCoverImagePushButton->setSwitchOn(AppSettings::ValueAsBool(kEnableBlurCover));
	(void)QObject::connect(ui_.enableBlurCoverImagePushButton, &SwitchButton::pressed, [this]() {
		AppSettings::SetValue(kEnableBlurCover,
			!AppSettings::ValueAsBool(kEnableBlurCover));
		});

    (void)QObject::connect(ui_.saveSoxrSettingBtn, &QPushButton::pressed, [this]() {
	    const auto setting_name = QInputDialog::getText(this, tr("Save soxr setting"), 
	                                                    tr("Setting name:"),
	                                                    QLineEdit::Normal,
	                                                    ui_.soxrSettingCombo->currentText());
        if (setting_name.isEmpty()) {
            return;
        }
        SaveSoxrResampler(setting_name);
		});

	(void)QObject::connect(ui_.deleteSoxrSettingBtn, &QPushButton::pressed, [this]() {
		const auto name = ui_.soxrSettingCombo->currentText();
		if (name == kSoxrDefaultSettingName) {
			return;
		}
        JsonSettings::remove(name);
        ui_.soxrSettingCombo->removeItem(ui_.soxrSettingCombo->currentIndex());
		SaveSoxrResampler(ui_.soxrSettingCombo->currentText());
		});

    (void)QObject::connect(ui_.soxrTargetSampleRateComboBox, &QComboBox::textActivated, [this](auto) {
		SaveSoxrResampler(ui_.soxrSettingCombo->currentText());
		});

    (void)QObject::connect(ui_.rollOffLevelComboBox, &QComboBox::textActivated, [this](auto) {
        SaveSoxrResampler(ui_.soxrSettingCombo->currentText());
    });

    (void)QObject::connect(ui_.soxrSettingCombo, &QComboBox::textActivated, [this](auto index) {
		const auto soxr_settings = JsonSettings::ValueAsMap(kSoxr);
		const auto settings = soxr_settings[index].toMap();
		UpdateSoxrConfigUi(settings);
		});
}

void PreferencePage::SetLanguage(int index) {
	const auto lang = LocaleLanguageManager::LanguageNames()[index];
	ui_.langCombo->setCurrentIndex(index);
	AppSettings::LoadLanguage(lang.GetIsoCode());
	AppSettings::SetValue(kAppSettingLang, lang.GetIsoCode());
	ui_.retranslateUi(this);
}

void PreferencePage::InitialLanguage() {
	const LocaleLanguage current_lang(AppSettings::ValueAsString(kAppSettingLang));

	auto current_index = 0;
	auto index = 0;

    Q_FOREACH (auto lang, LocaleLanguageManager::LanguageNames()) {
        QIcon ico(qTheme.GetCountryFlagFilePath(lang.CountryIsoCode().toUpper()));
		ui_.langCombo->addItem(ico, lang.NativeNameLang());
		if (current_lang.GetIsoCode() == lang.GetIsoCode()) {
			current_index = index;
		}
		index++;
	}

    ui_.langCombo->setCurrentIndex(current_index);

	(void)QObject::connect(ui_.langCombo, static_cast<void (QComboBox::*)(int32_t)>(&QComboBox::activated), 
		[this](auto const& index) {
		SetLanguage(index);
		});
}

void PreferencePage::SetPhasePercentText(int32_t value) {
	auto str = QString(qTEXT("%0%")).arg(value);
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

	InitSoxResampler();
	InitR8BrainResampler();
	InitPcm2Dsd();
	InitialLanguage();

	switch (qTheme.GetThemeColor()) {
	case ThemeColor::LIGHT_THEME:
		ui_.lightRadioButton->setChecked(true);
		break;
	case ThemeColor::DARK_THEME:
		ui_.darkRadioButton->setChecked(true);
		break;
	}

	(void)QObject::connect(ui_.lightRadioButton, &QRadioButton::clicked, [](auto checked) {
		emit qTheme.CurrentThemeChanged(ThemeColor::LIGHT_THEME);
		});

	(void)QObject::connect(ui_.darkRadioButton, &QRadioButton::clicked, [](auto checked) {
		emit qTheme.CurrentThemeChanged(ThemeColor::DARK_THEME);
		});

    ui_.preferenceTreeWidget->header()->hide();
    
    auto* settings_item = new QTreeWidgetItem(QStringList() << tr("Playback"));
    settings_item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);

    auto* dsp_manager_item = new QTreeWidgetItem(QStringList() << tr("Audio Resampler"));
    settings_item->addChild(dsp_manager_item);

	auto* pcm2dsd_item = new QTreeWidgetItem(QStringList() << tr("PCM/DSD Convert"));
	settings_item->addChild(pcm2dsd_item);

	// todo: 先不實作FoobarDSP相關UI
	/*auto* foobar2000_dsp_item = new QTreeWidgetItem(QStringList() << tr("Foobar2000 DSP"));
	settings_item->addChild(foobar2000_dsp_item);*/

    ui_.preferenceTreeWidget->addTopLevelItem(settings_item);
    ui_.preferenceTreeWidget->expandAll();
	ui_.preferenceTreeWidget->setCurrentItem(settings_item);

    (void)QObject::connect(ui_.preferenceTreeWidget, &QTreeWidget::itemClicked, [this](auto item, auto column) {
        const OrderedMap<QString, int32_t> stack_page_map{
            { tr("Playback"), 0 },
            { tr("Audio Resampler"), 1 },
			{ tr("PCM/DSD Convert"), 2 },
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
			AppSettings::SetValue(kAppSettingEnableReplayGain, true);
			break;
		case 1:
			AppSettings::setEnumValue(kAppSettingReplayGainMode, ReplayGainMode::RG_TRACK_MODE);
			AppSettings::SetValue(kAppSettingEnableReplayGain, true);
			break;
		case 2:
			AppSettings::setEnumValue(kAppSettingReplayGainMode, ReplayGainMode::RG_NONE_MODE);
			AppSettings::SetValue(kAppSettingEnableReplayGain, false);			
			break;
		}
		});

	ui_.replayGainModeCombo->setCurrentIndex(AppSettings::GetValue(kAppSettingReplayGainMode).toInt());

	(void)QObject::connect(ui_.selectResamplerComboBox, static_cast<void (QComboBox::*)(int32_t)>(&QComboBox::activated), [this](auto const& index) {
		ui_.resamplerStackedWidget->setCurrentIndex(index);
		switch (index) {
		case 1:
			SaveSoxrResampler(ui_.soxrSettingCombo->currentText());
			AppSettings::SetValue(kAppSettingResamplerType, kSoxr);
			break;
		case 2:
			SaveR8BrainResampler();
			AppSettings::SetValue(kAppSettingResamplerType, kR8Brain);
			break;
		}		
        SaveAll();
		});

    (void)QObject::connect(ui_.soxrPassbandSlider, &QSlider::valueChanged, [this](auto) {
        ui_.soxrPassbandValue->setText(QString(qTEXT("%0%")).arg(ui_.soxrPassbandSlider->value()));
		SaveSoxrResampler(ui_.soxrSettingCombo->currentText());
        SaveAll();
    });

    (void)QObject::connect(ui_.soxrPhaseSlider, &QSlider::valueChanged, [this](auto) {
		SetPhasePercentText(ui_.soxrPhaseSlider->value());
		SaveSoxrResampler(ui_.soxrSettingCombo->currentText());
        SaveAll();
    });

	(void)QObject::connect(ui_.resetAllButton, &QPushButton::clicked, [this]() {
		JsonSettings::remove(kSoxr);
		InitSoxResampler();
        SaveAll();
		});

    ui_.albumImageCacheSizeSpinBox->setValue(AppSettings::GetValue(kAppSettingAlbumImageCacheSize).toInt());
    (void)QObject::connect(ui_.albumImageCacheSizeSpinBox, static_cast<void (QSpinBox::*)(int32_t)>(&QSpinBox::valueChanged), [](auto value) {
        qPixmapCache.SetMaxSize(value);
        AppSettings::SetValue(kAppSettingAlbumImageCacheSize, value);
    });

	const QList<QWidget*> widgets {
		ui_.darkRadioButton,
		ui_.lightRadioButton,
		ui_.lbThemeMode,
		ui_.lbLang,
		ui_.lbFramelessWindow,
		ui_.lbBlurCoverImage,
		ui_.lbAlbumImageCacheSize,
		ui_.lbReplayGameMode,
	};

	const QList<QWidget*> pcm2dsd_page_widgets{
		ui_.enablePcm2DsdCheckBox,
		ui_.lbDsdTimes,
	};

	const QList<QWidget*> soxr_page_widgets{
		ui_.lbResampler,
		ui_.lbResamplerSettings,
		ui_.lbTargetSampleRate,
		ui_.lbQuality,
		ui_.soxrPassbandValue,
		ui_.soxrPhaseValue,
		ui_.lbHz,
		ui_.lbRollOffLevel,
	};

	const QList<QWidget*> r8brain_page_widgets{
		ui_.lbR8BrainTargetSampleRate,
		ui_.lbR8BrainHz,
	};

	QFont f(qTEXT("UIFont"));
	f.setPointSize(qTheme.GetFontSize(9));
	setFont(f);

	Q_FOREACH(auto *w, widgets) {
		QFont f(qTEXT("DisplayFont"));
		f.setWeight(QFont::DemiBold);
		f.setPointSize(qTheme.GetFontSize(14));

		if (dynamic_cast<QRadioButton*>(w) == nullptr && dynamic_cast<QCheckBox*>(w) == nullptr) {
			w->setFont(f);
		}
		w->setStyleSheet(qTEXT("background: transparent;"));
	}

	Q_FOREACH(auto* w, pcm2dsd_page_widgets) {
		w->setStyleSheet(qTEXT("background: transparent;"));
	}

	Q_FOREACH(auto* w, soxr_page_widgets) {
		w->setStyleSheet(qTEXT("background: transparent;"));
	}

	Q_FOREACH(auto* w, r8brain_page_widgets) {
		w->setStyleSheet(qTEXT("background: transparent;"));
	}

	ui_.stackedWidget->setCurrentIndex(0);
	setFixedSize(900, 700);	
}

void PreferencePage::LoadSettings() {
	const auto enable_resampler = AppSettings::ValueAsBool(kAppSettingResamplerEnable);
	if (!enable_resampler) {
		ui_.resamplerStackedWidget->setCurrentIndex(0);
		ui_.selectResamplerComboBox->setCurrentIndex(0);
	}
	else {
		auto resampler_type = AppSettings::ValueAsString(kAppSettingResamplerType);
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

void PreferencePage::SaveAll() {
	SaveSoxrResampler(ui_.soxrSettingCombo->currentText());
	SaveR8BrainResampler();

	auto index = ui_.resamplerStackedWidget->currentIndex();
	AppSettings::SetValue(kAppSettingResamplerEnable, index > 0);

	JsonSettings::save();
	AppSettings::save();
}
