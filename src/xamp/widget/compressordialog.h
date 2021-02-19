//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QDialog>

#include "ui_compressordialog.h"

class CompressorDialog final : public QDialog {
	Q_OBJECT
public:
    explicit CompressorDialog(QWidget* parent = nullptr);

signals:
    void onValueChange(float gain, float threshold, float ratio, float attack, float release);

private:
    void notifyChange();
	
    Ui::CompressorDialog ui_;
};
