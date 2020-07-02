#include <QDir>
#include <QDirIterator>
#include <QTextStream>

#include <widget/str_utilts.h>

#include "eqdialog.h"
#include "ui_eqdialog.h"

EQDialog::EQDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::EQDialog) {
    ui->setupUi(this);

    bands_ = QList<UIBand>{
        { ui->band1Slider, ui->band1Edit },
        { ui->band2Slider, ui->band2Edit },
        { ui->band3Slider, ui->band3Edit },
        { ui->band4Slider, ui->band4Edit },
        { ui->band5Slider, ui->band5Edit },
        { ui->band6Slider, ui->band6Edit },
        { ui->band7Slider, ui->band7Edit },
        { ui->band8Slider, ui->band8Edit },
        { ui->band9Slider, ui->band9Edit },
        { ui->band10Slider, ui->band10Edit },
    };

    newBandSettings.push_back({ 0, 0 });
    newBandSettings.push_back({ 0, 0 });
    newBandSettings.push_back({ 0, 0 });
    newBandSettings.push_back({ 0, 0 });
    newBandSettings.push_back({ 0, 0 });
    newBandSettings.push_back({ 0, 0 });
    newBandSettings.push_back({ 0, 0 });
    newBandSettings.push_back({ 0, 0 });
    newBandSettings.push_back({ 0, 0 });
    newBandSettings.push_back({ 0, 0 });

    for (auto i = 0; i < bands_.size(); ++i) {
       (void)QObject::connect(bands_[i].slider, &QSlider::valueChanged,
           [this, i](auto value) { 
           bands_[i].edit->setText(QString::number(value / 100.0F, 'f', 2));
           newBandSettings[i].gain = value / 100.0F;
       });
    }

    for (auto itr = AppSettings::EQBands.begin();
         itr != AppSettings::EQBands.end();
         ++itr) {
        ui->presetsComboBox->addItem(itr.key());
    }    

    (void)QObject::connect(ui->presetsComboBox,
                            static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::activated),
                            [this](auto EQName) {
        eqName = EQName;
        updateBar(EQName);
    });

    (void)QObject::connect(ui->enableEQCheckBox, &QCheckBox::stateChanged, [this](auto state) {
        if (state == Qt::Checked) {
            eqName = ui->presetsComboBox->currentText();
            AppSettings::setValue(kEnableEQ, true);
            updateBar(eqName);
        } else {
            eqName = QEmptyString;
            AppSettings::setValue(kEnableEQ, false);
        }
    });

    if (AppSettings::EQBands.size() == 1) {
        updateBar(AppSettings::EQBands.firstKey());
        eqName = AppSettings::EQBands.firstKey();
    }    

    if (!AppSettings::getValueAsString(kEQName).isEmpty()) {
        ui->presetsComboBox->setCurrentText(AppSettings::getValueAsString(kEQName));        
        updateBar(AppSettings::getValueAsString(kEQName));
    }

    if (AppSettings::getValueAsBool(kEnableEQ)) {
        ui->enableEQCheckBox->setCheckState(Qt::Checked);
    }
}

void EQDialog::updateBar(const QString& name) {   
    auto i = 0;
    for (auto bar : AppSettings::EQBands[name]) {
        bands_[i].slider->setValue(bar.gain * 100);
        bands_[i].edit->setText(QString::number(bar.gain));
        newBandSettings[i].gain = bar.gain;
        newBandSettings[i].Q = bar.Q;
        ++i;
    }

    AppSettings::setValue(kEQName, name);
}

EQDialog::~EQDialog() {
    delete ui;
}
