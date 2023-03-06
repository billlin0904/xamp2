//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <ui_aboutdialog.h>

#include <thememanager.h>

class AboutPage final : public QFrame {
    Q_OBJECT
public:
    explicit AboutPage(QWidget* parent = nullptr);

public slots:
    void OnCreditsOrLicenceChecked(bool checked);

    void OnCurrentThemeChanged(ThemeColor theme_color);

private:
    Ui::AboutDialog ui;
    QString lincense_;
    QString credits_;
};
