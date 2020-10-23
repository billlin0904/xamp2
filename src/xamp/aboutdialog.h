//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <widget/xampdialog.h>
#include <ui_aboutdialog.h>

class AboutDialog final : public XampDialog {
    Q_OBJECT
public:
    explicit AboutDialog(QWidget* parent = nullptr);

private slots:
    void onCreditsOrLicenceChecked(bool checked);

private:
    Ui::AboutDialog ui;
    QString lincense_;
    QString credits_;
};
