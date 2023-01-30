//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <ui_aboutdialog.h>

class AboutPage final : public QFrame {
    Q_OBJECT
public:
    explicit AboutPage(QWidget* parent = nullptr);

private slots:
    void OnCreditsOrLicenceChecked(bool checked);

private:
    Ui::AboutDialog ui;
    QString lincense_;
    QString credits_;
};
