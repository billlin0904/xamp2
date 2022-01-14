//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QLabel>
#include <QLayout>
#include <QFrame>

#include <widget/xdialog.h>

struct XMessageBox {
public:
	static void about(const QString &msg, QWidget *parent) {
        auto *text = new QLabel(parent);
        text->setText(msg);
        XDialog dialog(parent);
        dialog.setContentWidget(text);
        dialog.exec();
	}
};
