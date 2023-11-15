//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QTabWidget>
#include <widget/widget_shared_global.h>

class QMouseEvent;

class XAMP_WIDGET_SHARED_EXPORT PlaylistTabWidget : public QTabWidget {
	Q_OBJECT
public:
	explicit PlaylistTabWidget(QWidget* parent = nullptr);

signals:
	void CreateNewPlaylist();

private:
	void mouseDoubleClickEvent(QMouseEvent* e) override;
};

