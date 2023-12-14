//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QMap>
#include <QFrame>

#include <widget/widget_shared_global.h>
#include <widget/widget_shared.h>

namespace Ui {
    class PreferenceDialog;
}

class XAMP_WIDGET_SHARED_EXPORT PreferencePage final : public QFrame {
    Q_OBJECT
public:
    explicit PreferencePage(QWidget *parent = nullptr);

    virtual ~PreferencePage() override;

    void loadSettings();

    void saveAll();

private:
    void updateSoxrConfigUi(const QVariantMap& soxr_settings);

    void saveSoxrResampler(const QString &name) const;

    void saveR8BrainResampler();

    void saveSrcResampler();

    void initSrcResampler();

    void initR8BrainResampler();

	void initSoxResampler();

    void setLanguage(int index);

    void initialLanguage();    

    QMap<QString, QVariant> currentSoxrSettings() const;

    void setPhasePercentText(int32_t value);

    Ui::PreferenceDialog* ui_;
};
