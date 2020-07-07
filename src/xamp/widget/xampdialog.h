//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QDialog>

class XampDialog : public QDialog {
    Q_OBJECT
public:
    explicit XampDialog(QWidget* parent = nullptr);

private:
    void centerWidgets(QWidget* widget);
};

