//=====================================================================================================================
// Copyright (c) 2018-2025 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFrame>
#include <QDateTime>
#include <widget/widget_shared_global.h>

namespace Ui {
	class AccountAuthorizationPage;
}

class XAMP_WIDGET_SHARED_EXPORT AccountAuthorizationPage : public QFrame {
public:
	explicit AccountAuthorizationPage(QWidget* parent = nullptr);

	virtual ~AccountAuthorizationPage() override;

private:
	Ui::AccountAuthorizationPage* ui_;
};

