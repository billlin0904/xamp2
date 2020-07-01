#include <QDir>
#include <QDirIterator>
#include <QTextStream>

#include <widget/appsettings.h>
#include <widget/str_utilts.h>

#include "eqdialog.h"
#include "ui_eqdialog.h"

EQDialog::EQDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::EQDialog) {
    ui->setupUi(this);

    for (auto itr = AppSettings::allBands.begin();
         itr != AppSettings::allBands.end();
         ++itr) {
        ui->presetsComboBox->addItem(itr.key());
    }

    QList<UIBand> bandSliders {
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

    (void)QObject::connect(ui->presetsComboBox,
                            static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::activated),
                            [this](auto EQName) {
        selectedEQName = EQName;
    });

    (void)QObject::connect(ui->enableEQCheckBox, &QCheckBox::stateChanged, [this](auto state) {
        if (state == Qt::Checked) {
            selectedEQName = ui->presetsComboBox->currentText();
            AppSettings::setValue(kEnableEQ, true);
            AppSettings::setValue(kEQName, selectedEQName);
        } else {
            selectedEQName = QEmptyString;
            AppSettings::setValue(kEnableEQ, false);
        }
    });

    if (AppSettings::allBands.size() == 1) {
        auto i = 0;
        for (auto bar : AppSettings::allBands.first()) {
            bandSliders[i].slider->setValue(bar.gain);
            bandSliders[i].editor->setText(QString::number(bar.gain));
            ++i;
        }
        selectedEQName = AppSettings::allBands.firstKey();
    }

    if (AppSettings::getValueAsBool(kEnableEQ)) {
        ui->enableEQCheckBox->setCheckState(Qt::Checked);
    }

    if (!AppSettings::getValueAsString(kEQName).isEmpty()) {
        ui->presetsComboBox->setCurrentText(AppSettings::getValueAsString(kEQName));
    }
}

EQDialog::~EQDialog() {
    delete ui;
}
