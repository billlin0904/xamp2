//=====================================================================================================================
// Copyright (c) 2018-2025 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QLineEdit>
#include <QList>
#include <QTimer>
#include <QLabel>

#include <widget/widget_shared.h>
#include <widget/widget_shared_global.h>
#include <stream/bassparametriceq.h>

class DoubleSlider;

namespace Ui {
    class EqualizerView;
}

class XAMP_WIDGET_SHARED_EXPORT EqualizerView final : public QFrame {
    Q_OBJECT
public:
    explicit EqualizerView(QWidget *parent = nullptr);

    virtual ~EqualizerView() override;

signals:
   void bandValueChanged(int band, float value, float Q);

   void preampValueChanged(float value);

public slots:

private:
    void applySetting(const QString &name, const EqSettings &settings);

    std::vector<QLabel*> freq_label_;
    std::vector<QLabel*> bands_label_;
    std::vector<DoubleSlider*> sliders_;
    Ui::EqualizerView *ui_;
};
