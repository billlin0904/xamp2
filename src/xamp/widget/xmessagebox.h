//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QLabel>
#include <widget/str_utilts.h>
#include <widget/xdialog.h>

struct XMessageBox {
	static void about(QWidget* parent, const QString &msg, const QString& title= kAppTitle) {
        auto *text = new QLabel();
        text->setText(msg);
        auto* dialog = new XDialog(parent);
        dialog->setContentWidget(text);
        dialog->setTitle(title);
        dialog->exec();
	}
};
