//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QDialog>

#include "ui_preferencedialog.h"

class PreferenceDialog : public QDialog {
public:
    explicit PreferenceDialog(QWidget *parent = nullptr);

    QString musicFilePath;

private:
    void loadSoxrResampler(const QVariantMap & soxr_settings);

    void saveSoxrResampler(const QString &name);

	void initSoxResampler();

    void initLang();

    Ui::PreferenceDialog ui_;
};
