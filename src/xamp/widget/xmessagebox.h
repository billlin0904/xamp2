//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QLabel>
#include <widget/str_utilts.h>
#include <widget/xdialog.h>

struct XMessageBox {
	static void about(const QString &msg, QWidget *parent, const QString& title= kAppTitle) {
        auto *text = new QLabel(parent);
        text->setText(msg);
        XDialog dialog(parent);
        dialog.setContentWidget(text);
        dialog.setTitle(title);
        dialog.exec();
	}
};
