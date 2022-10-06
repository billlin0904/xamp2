//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>
#include <QScopedPointer>
#include "thememanager.h"

class ThemeNotifier : public QObject {
	Q_OBJECT
public:
	explicit ThemeNotifier(QObject* parent = nullptr);

	~ThemeNotifier();

signals:
	void themeChanged(ThemeColor theme_color);

private:
	class ThemeNotifierImpl;
	QScopedPointer<ThemeNotifierImpl> impl_;
};