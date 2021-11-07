#include <QDir>
#include <QTextStream>
#include <QDirIterator>
#include <widget/str_utilts.h>
#include <widget/equalizerdialog.h>

EqualizerDialog::EqualizerDialog(QWidget *parent)
    : QDialog(parent) {
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

     auto band = 0;
     for (auto& slider : band_sliders) {
         (void)QObject::connect(slider, &DoubleSlider::doubleValueChanged, [band, this](auto value) {
             bandValueChange(band, value, 1.0);
         });
         ++band;
     }

     (void)QObject::connect(ui_.preampSlider, &DoubleSlider::doubleValueChanged, [this](auto value) {
         preampValueChange(value);
     });

     parseEqFile();

     (void)QObject::connect(ui_.eqPresetComboBox, static_cast<void (QComboBox::*)(const QString&)>(&QComboBox::activated), [band_sliders, this](auto index) {
         auto setting = settings_[index];
         for (auto i = 0; i < setting.bands.size(); ++i) {
             band_sliders[i]->setValue(static_cast<int>(setting.bands[i].gain));
         }
         ui_.preampSlider->setValue(static_cast<int>(setting.preamp));
     });

     ui_.eqPresetComboBox->setCurrentIndex(0);
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
