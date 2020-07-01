//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QDialog>
#include <QMap>
#include <QSlider>
#include <QLineEdit>

namespace Ui {
class EQDialog;
}

struct UIBand {
    QSlider *slider;
    QLineEdit *editor;
};

class EQDialog : public QDialog {
    Q_OBJECT

public:
    explicit EQDialog(QWidget *parent = nullptr);
    ~EQDialog();

    QString selectedEQName;

private:
    Ui::EQDialog *ui;
};
