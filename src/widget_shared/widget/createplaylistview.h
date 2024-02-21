//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QFrame>

#include <widget/widget_shared_global.h>

namespace Ui {
	class CreatePlaylistView;
}

class XAMP_WIDGET_SHARED_EXPORT CreatePlaylistView : public QFrame {
	Q_OBJECT
public:
	explicit CreatePlaylistView(QWidget* parent = nullptr);

	virtual ~CreatePlaylistView() override;

	QString title() const;

	QString desc() const;

	int32_t privateStatus() const;

private:
	Ui::CreatePlaylistView* ui_;
};

