//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QMap>

#include "ui_preferencedialog.h"

class PreferencePage final : public QFrame {
public:
    explicit PreferencePage(QWidget *parent = nullptr);

    void update();

private:
    void updateSoxrConfigUI(const QVariantMap& soxr_settings);

    void saveSoxrResampler(const QString &name) const;

	void initSoxResampler();

    void setLang(int index);

    void initLang();

    void saveAll();

    QMap<QString, QVariant> currentSoxrSettings() const;

    void setPhasePercentText(int32_t value);
    Ui::PreferenceDialog ui_;
};
