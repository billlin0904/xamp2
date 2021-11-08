#include <QDir>
#include <QTextStream>
#include <QDirIterator>
#include <widget/appsettings.h>
#include <widget/str_utilts.h>
#include <widget/equalizerdialog.h>

EqualizerDialog::EqualizerDialog(QWidget *parent)
    : XampDialog(parent) {
     ui_.setupUi(this);

     std::vector<DoubleSlider*> band_sliders {
         ui_.band1Slider,
         ui_.band2Slider,
         ui_.band3Slider,
         ui_.band4Slider,
         ui_.band5Slider,
         ui_.band6Slider,
         ui_.band7Slider,
         ui_.band8Slider,
         ui_.band9Slider,
         ui_.band10Slider,
     };

     std::vector<QLabel*> band_label{
         ui_.band1DbLabel,
         ui_.band2DbLabel,
         ui_.band3DbLabel,
         ui_.band4DbLabel,
         ui_.band5DbLabel,
         ui_.band6DbLabel,
         ui_.band7DbLabel,
         ui_.band8DbLabel,
         ui_.band9DbLabel,
         ui_.band10DbLabel,
     };

     QFont f(Q_UTF8("MonoFont"));
     f.setPointSize(12);
     for (auto& l : band_label) {
         l->setFont(f);
     }

     auto band = 0;
     for (auto& slider : band_sliders) {
         (void)QObject::connect(slider, &DoubleSlider::doubleValueChanged, [band, band_label, this](auto value) {
             bandValueChange(band, value, 1.41);
             band_label[band]->setText(QString(Q_UTF8("%1 db")).arg(value * 10));

             AppEQSettings settings;
             settings.name = ui_.eqPresetComboBox->currentText();
             settings.settings = settings_[ui_.eqPresetComboBox->currentText()];
             settings.settings.bands[band].gain = value;
             AppSettings::setValue(kEQName, QVariant::fromValue(settings));
         });
         ++band;
     }

     (void)QObject::connect(ui_.preampSlider, &DoubleSlider::doubleValueChanged, [this](auto value) {
         preampValueChange(value);
     });

     parseEqFile();

     (void)QObject::connect(ui_.eqPresetComboBox, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::activated), [band_sliders, this](auto index) {
         AppEQSettings settings;
         settings.name = index;
         settings.settings = settings_[index];
         AppSettings::setValue(kEQName, QVariant::fromValue(settings));
         applySetting(index);
         AppSettings::save();
     });

     if (AppSettings::contains(kEQName)) {
         auto setting = AppSettings::getValue(kEQName).value<AppEQSettings>();
         ui_.eqPresetComboBox->setCurrentText(setting.name);
         applySetting(setting.name);
     }
}

void EqualizerDialog::applySetting(QString const& name) {
    std::vector<DoubleSlider*> band_sliders{
         ui_.band1Slider,
         ui_.band2Slider,
         ui_.band3Slider,
         ui_.band4Slider,
         ui_.band5Slider,
         ui_.band6Slider,
         ui_.band7Slider,
         ui_.band8Slider,
         ui_.band9Slider,
         ui_.band10Slider,
    };

    std::vector<QLabel*> band_label{
        ui_.band1DbLabel,
        ui_.band2DbLabel,
        ui_.band3DbLabel,
        ui_.band4DbLabel,
        ui_.band5DbLabel,
        ui_.band6DbLabel,
        ui_.band7DbLabel,
        ui_.band8DbLabel,
        ui_.band9DbLabel,
        ui_.band10DbLabel,
    };

    auto setting = settings_[name];
    for (auto i = 0; i < setting.bands.size(); ++i) {
        band_sliders[i]->setValue(setting.bands[i].gain * 10);
        band_label[i]->setText(QString(Q_UTF8("%1 db")).arg(setting.bands[i].gain * 10));
    }
    ui_.preampSlider->setValue(setting.preamp * 10);
}

void EqualizerDialog::parseEqFile() {
    auto path = QDir::currentPath() + Q_UTF8("/eqpresets/");
    auto file_ext = QStringList() << Q_UTF8("*.*");
    for (QDirIterator itr(path, file_ext, QDir::Files | QDir::NoDotAndDotDot);
         itr.hasNext();) {
        const auto filepath = itr.next();
        const QFileInfo file_info(filepath);
        QFile file(filepath);
        if (file.open(QIODevice::ReadOnly)) {
            QTextStream in(&file);
            EQSettings settings;
            int i = 0;
            while (!in.atEnd()) {
                auto line = in.readLine();
                auto result = line.split(Q_UTF8(":"));
                auto str = result[1].toStdWString();
                if (result[0] == Q_UTF8("Preamp")) {
                    swscanf(str.c_str(), L"%f dB",
                                             &settings.preamp);
                } else if (result[0].indexOf(Q_UTF8("Filter") != -1)) {
                    auto pos = str.find(L"Gain");
                    if (pos == std::wstring::npos) {
                        continue;
                    }
                    swscanf(&str[pos], L"Gain %f dB Q %f",
                       &settings.bands[i].gain, &settings.bands[i].Q);
                    ++i;
                }
            }
            settings_[file_info.baseName()] = settings;
            ui_.eqPresetComboBox->addItem(file_info.baseName());
        }
    }
}
