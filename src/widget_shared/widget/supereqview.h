//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QLineEdit>
#include <QList>
#include <QTimer>
#include <QLabel>
#include <QMap>

#include <widget/widget_shared.h>
#include <widget/widget_shared_global.h>
#include <stream/bassparametriceq.h>

class QSlider;

namespace Ui {
    class SuperEqView;
}

class XAMP_WIDGET_SHARED_EXPORT SuperEqView : public QFrame {
    Q_OBJECT
public:
    explicit SuperEqView(QWidget* parent = nullptr);

    virtual ~SuperEqView() override;

    const EqSettings& getCurrentSetting() const;

signals:
    void bandValueChange(int band, float value, float Q);

    void preampValueChange(float value);

public slots:

private:
    void applySetting(const QString& name, const EqSettings& settings);

    Ui::SuperEqView* ui_;
    EqSettings current_settings_;
    QMap<QString, EqSettings> settingses_;
    std::vector<QLabel*> freq_label_;
    std::vector<QLabel*> bands_label_;
    std::vector<QSlider*> sliders_;
};
