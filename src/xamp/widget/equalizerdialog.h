//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QMap>

#include <stream/iequalizer.h>
#include <widget/xampdialog.h>

#include "ui_equalizerdialog.h"

using xamp::stream::EQSettings;

class EqualizerDialog : public XampDialog {
    Q_OBJECT
public:
    explicit EqualizerDialog(QWidget *parent = nullptr);

signals:
   void bandValueChange(int band, float value, float Q);

   void preampValueChange(float value);

private:
    void parseEqFile();

    void applySetting(QString const &name);

    Ui::EqualizerDialog ui_;
    QMap<QString, EQSettings> settings_;
};
