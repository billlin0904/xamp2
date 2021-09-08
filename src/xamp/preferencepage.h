//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QDialog>
#include <QMap>

#include "ui_preferencedialog.h"
#include <widget/xampdialog.h>

class PreferencePage final : public QFrame {
public:
    explicit PreferencePage(QWidget *parent = nullptr);

private:
    void loadSoxrResampler(const QVariantMap & soxr_settings);

    void saveSoxrResampler(const QString &name);

	void initSoxResampler();

    void initLang();

    QMap<QString, QVariant> getSoxrSettings() const;

    int32_t soxr_passband_;
    int32_t soxr_phase_;
    Ui::PreferenceDialog ui_;
};
