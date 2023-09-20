//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QLineEdit>
#include <QList>
#include <QTimer>

#include <ui_equalizerdialog.h>
#include <widget/widget_shared.h>
#include <widget/widget_shared_global.h>
#include <stream/bassparametriceq.h>

class DoubleSlider;

class XAMP_WIDGET_SHARED_EXPORT EqualizerView : public QFrame {
    Q_OBJECT
public:
    explicit EqualizerView(QWidget *parent = nullptr);

signals:
   void BandValueChange(int band, float value, float Q);

   void PreampValueChange(float value);

public slots:

private:
    void ApplySetting(const QString &name, const EqSettings &settings);

    std::array<QLabel*, 10> freq_label_;
    std::array<QLabel*, 10> bands_label_;
    std::array<DoubleSlider*, 10> sliders_;
    Ui::EqualizerView ui_;
};
