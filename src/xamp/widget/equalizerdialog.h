//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <stream/iequalizer.h>

#include "ui_equalizerdialog.h"

using xamp::stream::EQSettings;

class EqualizerDialog : public QFrame {
    Q_OBJECT
public:
    explicit EqualizerDialog(QWidget *parent = nullptr);

signals:
   void bandValueChange(int band, float value, float Q);

   void preampValueChange(float value);

private:
    void parseEqFile();

    void applySetting(QString const &name, EQSettings const &settings);

    Ui::EqualizerDialog ui_;
    std::vector<DoubleSlider*> band_sliders_;
    std::vector<QLabel*> band_label_;
};
