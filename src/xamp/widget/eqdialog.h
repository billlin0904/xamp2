//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QDialog>
#include <QMap>
#include <QSlider>
#include <QLineEdit>

#include <widget/appsettings.h>

namespace Ui {
class EQDialog;
}

class EQDialog : public QDialog {
    Q_OBJECT

public:
    explicit EQDialog(QWidget *parent = nullptr);
    ~EQDialog();

    QList<FilterBand> newBandSettings;
    QString eqName;

private:
    struct UIBand {
        QSlider* slider;
        QLineEdit* edit;
    };

    void updateBar(const QString& name);
    
    Ui::EQDialog *ui;
    QList<UIBand> bands_;
};