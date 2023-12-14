#include <QDirIterator>
#include <ui_supereqdialog.h>

#include <widget/supereqview.h>

#include <widget/appsettingnames.h>
#include <widget/appsettings.h>
#include <widget/str_utilts.h>
#include <widget/widget_shared.h>
#include <stream/eqsettings.h>
#include <ui_equalizerdialog.h>
#include <thememanager.h>

namespace {
	QMap<QString, EqSettings> GetEqPresets() {
		const auto path = QDir::currentPath() + qTEXT("/Equalizer Presets/");
		const auto file_ext = QStringList() << qTEXT("*.feq");
        QMap<QString, EqSettings> result;

        for (QDirIterator itr(path, file_ext, QDir::Files | QDir::NoDotAndDotDot);
            itr.hasNext();) {
            const auto filepath = itr.next();
            const QFileInfo file_info(filepath);

        	QFile file(filepath);

            if (file.open(QIODevice::ReadOnly)) {
	            EqSettings settings;
	            QTextStream in(&file);
                in.setEncoding(QStringConverter::Utf8);
                while (!in.atEnd()) {
                    auto line = in.readLine();
                    EqBandSetting setting;
                    setting.gain = line.toInt();
                    settings.bands.push_back(setting);
                }
                result.insert(file_info.baseName(), settings);
            }
        }
        return result;
	}
}

SuperEqView::SuperEqView(QWidget* parent)
    : QFrame(parent) {
    ui_ = new Ui::SuperEqView();
    ui_->setupUi(this);

    freq_label_ = std::vector<QLabel*>{
       ui_->band1FeqLabel,
       ui_->band2FeqLabel,
       ui_->band3FeqLabel,
       ui_->band4FeqLabel,
       ui_->band5FeqLabel,
       ui_->band6FeqLabel,
       ui_->band7FeqLabel,
       ui_->band8FeqLabel,
       ui_->band9FeqLabel,
       ui_->band10FeqLabel,
       ui_->band11FeqLabel,
       ui_->band12FeqLabel,
       ui_->band13FeqLabel,
       ui_->band14FeqLabel,
       ui_->band15FeqLabel,
       ui_->band16FeqLabel,
       ui_->band17FeqLabel,
       ui_->band18FeqLabel,
    };

    sliders_ = std::vector<QSlider*>{
        ui_->band1Slider,
        ui_->band2Slider,
        ui_->band3Slider,
        ui_->band4Slider,
        ui_->band5Slider,
        ui_->band6Slider,
        ui_->band7Slider,
        ui_->band8Slider,
        ui_->band9Slider,
        ui_->band10Slider,
        ui_->band11Slider,
        ui_->band12Slider,
        ui_->band13Slider,
        ui_->band14Slider,
        ui_->band15Slider,
        ui_->band16Slider,
        ui_->band17Slider,
        ui_->band18Slider,
    };

    bands_label_ = std::vector<QLabel*>{
        ui_->band1DbLabel,
        ui_->band2DbLabel,
        ui_->band3DbLabel,
        ui_->band4DbLabel,
        ui_->band5DbLabel,
        ui_->band6DbLabel,
        ui_->band7DbLabel,
        ui_->band8DbLabel,
        ui_->band9DbLabel,
        ui_->band10DbLabel,
    	ui_->band11DbLabel,
        ui_->band12DbLabel,
        ui_->band13DbLabel,
        ui_->band14DbLabel,
        ui_->band15DbLabel,
        ui_->band16DbLabel,
        ui_->band17DbLabel,
        ui_->band18DbLabel,
    };

    auto f = qTheme.monoFont();
    f.setPointSize(qTheme.fontSize(8));

    for (auto& l : freq_label_) {
        l->setStyleSheet(qTEXT("background-color: transparent;"));
        l->setFont(f);
    }

    for (auto& l : bands_label_) {
        l->setStyleSheet(qTEXT("background-color: transparent;"));
        l->setFont(f);
    }

    for (auto& s : sliders_) {
        s->setRange(-20, 20);
    }

    ui_->enableEqCheckBox->setCheckState(qAppSettings.valueAsBool(kAppSettingEnableEQ)
        ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);

    (void)QObject::connect(ui_->enableEqCheckBox, &QCheckBox::stateChanged, [this](auto value) {
        qAppSettings.setValue(kAppSettingEnableEQ, value == Qt::CheckState::Checked);
        });

    (void)QObject::connect(ui_->enableEqCheckBox, &QCheckBox::stateChanged, [this](auto value) {
        qAppSettings.setValue(kAppSettingEnableEQ, value == Qt::CheckState::Checked);
        });

    (void)QObject::connect(ui_->eqPresetComboBox, &QComboBox::textActivated, [this](auto index) {
        current_settings_ = settingses_[index];
        ApplySetting(index, current_settings_);
        });

    settingses_ = GetEqPresets();
    for (auto& name : settingses_.keys()) {
        ui_->eqPresetComboBox->addItem(name);
    }

    if (qAppSettings.contains(kAppSettingEQName)) {
        auto [name, settings] = qAppSettings.eqSettings();
        ApplySetting(name, settings);
    } else {
        ApplySetting(settingses_.firstKey(), settingses_.first());
    }
}

const EqSettings& SuperEqView::GetCurrentSetting() const {
    return current_settings_;
}

SuperEqView::~SuperEqView() {
    delete ui_;
}

void SuperEqView::ApplySetting(const QString& name, const EqSettings& settings) {
    for (auto i = 0; i < settings.bands.size(); ++i) {
        bands_label_[i]->setText(FormatDb(settings.bands[i].gain, 2));
        sliders_[i]->setValue(settings.bands[i].gain);
        bands_label_[i]->show();
        sliders_[i]->show();
    }
    AppEQSettings app_settings;
    app_settings.name = name;
    app_settings.settings = settings;
    qAppSettings.setEqSettings(app_settings);
    qAppSettings.save();
    ui_->eqPresetComboBox->setCurrentText(name);
}