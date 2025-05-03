//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <widget/widget_shared_global.h>
#include <widget/util/str_util.h>
#include <thememanager.h>

namespace Ui {
    class AboutDialog;
}

class XAMP_WIDGET_SHARED_EXPORT AboutPage final : public QFrame {
    Q_OBJECT
public:
    explicit AboutPage(QWidget* parent = nullptr);

    virtual ~AboutPage() override;

signals:
    void CheckForUpdate();

    void RestartApp();

public slots:
    void OnCreditsOrLicenseChecked(bool checked);

    void onThemeChangedFinished(ThemeColor theme_color);

    void OnUpdateNewVersion(const QVersionNumber& version);
private:
    QString license_;
    QString credits_;
    Ui::AboutDialog *ui_;
};
