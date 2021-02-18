//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QDialog>
#include <QMap>

#include "ui_preferencedialog.h"

class PreferenceDialog final : public QDialog {
public:
    explicit PreferenceDialog(QWidget *parent = nullptr);

    QString music_file_path_;

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
