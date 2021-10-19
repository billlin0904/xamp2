//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QApplication>
#include <QFile>
#include <QFont>
#include <QProxyStyle>
#include <QStyleFactory>

class DarkStyle : public QProxyStyle {
	Q_OBJECT

public:
	DarkStyle();
	explicit DarkStyle(QStyle* style);

	QStyle* baseStyle() const;

	void polish(QPalette& palette) override;
	void polish(QApplication* app) override;

private:
	QStyle* styleBase(QStyle* style = Q_NULLPTR) const;
};

