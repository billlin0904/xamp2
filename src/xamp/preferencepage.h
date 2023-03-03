//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QMap>

#include "ui_preferencedialog.h"

#include <widget/widget_shared.h>

class PreferencePage final : public QFrame {
    Q_OBJECT
public:
    explicit PreferencePage(QWidget *parent = nullptr);

    void LoadSettings();

    void SaveAll();

private:
    void UpdateSoxrConfigUi(const QVariantMap& soxr_settings);

    void SavePcm2Dsd();

    void SaveSoxrResampler(const QString &name) const;

    void SaveR8BrainResampler();

    void InitR8BrainResampler();

	void InitSoxResampler();

    void InitPcm2Dsd();

    void SetLanguage(int index);

    void InitialLanguage();    

    QMap<QString, QVariant> CurrentSoxrSettings() const;

    void SetPhasePercentText(int32_t value);

    Ui::PreferenceDialog ui_;
};
