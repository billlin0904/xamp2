//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <ui_aboutdialog.h>
#include <widget/widget_shared_global.h>
#include <thememanager.h>

class XAMP_WIDGET_SHARED_EXPORT AboutPage final : public QFrame {
    Q_OBJECT
public:
    explicit AboutPage(QWidget* parent = nullptr);

public slots:
    void OnCreditsOrLicenseChecked(bool checked);

    void OnCurrentThemeChanged(ThemeColor theme_color);

private:
    Ui::AboutDialog ui_;
    QString license_;
    QString credits_;
};
