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

    void LoadSettings();

    void SaveAll();

private:
    void UpdateSoxrConfigUi(const QVariantMap& soxr_settings);

    void SaveSoxrResampler(const QString &name) const;

    void SaveR8BrainResampler();

    void SaveSrcResampler();

    void InitSrcResampler();

    void InitR8BrainResampler();

	void InitSoxResampler();

    void SetLanguage(int index);

    void InitialLanguage();    

    QMap<QString, QVariant> CurrentSoxrSettings() const;

    void SetPhasePercentText(int32_t value);

    Ui::PreferenceDialog* ui_;
};
