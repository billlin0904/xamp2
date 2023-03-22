//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QLineEdit>
#include <QList>
#include <QTimer>
#include <widget/widget_shared.h>
#include <stream/bassparametriceq.h>
#include "ui_equalizerdialog.h"

class ParametricEqView;

class EqualizerDialog : public QFrame {
    Q_OBJECT
public:
    explicit EqualizerDialog(QWidget *parent = nullptr);

signals:
   void BandValueChange(int band, float value, float Q);

   void PreampValueChange(float value);

public slots:
    void OnFftResultChanged(const ComplexValarray & result);

private:
    void ApplySetting(QString const &name, EQSettings const &settings);

    void AddBand(uint32_t band);

    void SetBandGain(uint32_t band, float gain);

    void SetBand(uint32_t band, EQFilterTypes type, float frequency, float gain, float Q, float band_width);

    void RemoveBand(uint32_t band) const;

    void Save(uint32_t band);

    struct FilterSetting {
        QLabel* band;
        QComboBox* type;
        QLineEdit* frequency;
        QLineEdit* gain;
        QLineEdit* Q;
        QLineEdit* band_width;
    };

    int32_t num_band_ = 1;
    ParametricEqView* parametric_eq_view_;
    QList<DoubleSlider*> band_sliders_;
    QList<QLabel*> band_label_;
    QList<QLabel*> band_feq_label_;
    std::array<FilterSetting, kEQMaxBand> filter_settings_;
    QTimer timer_;
    ComplexValarray fft_result_;
    Ui::EqualizerDialog ui_;
};
