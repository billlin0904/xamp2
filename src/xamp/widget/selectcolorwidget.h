//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QWidget>
#include <QColor>
#include <QList>

class SelectColorWidget : public QWidget {
    Q_OBJECT
public:
    explicit SelectColorWidget(QWidget* parent = nullptr);

signals:
    void colorButtonClicked(const QColor& clr);
};

