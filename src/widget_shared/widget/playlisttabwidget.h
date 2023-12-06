//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QTabWidget>
#include <QMap>
#include <widget/widget_shared_global.h>

class QMouseEvent;

class XAMP_WIDGET_SHARED_EXPORT PlaylistTabWidget : public QTabWidget {
	Q_OBJECT
public:
	static constexpr QSize kTabIconSize = QSize(32, 32);

	explicit PlaylistTabWidget(QWidget* parent = nullptr);

	int32_t GetCurrentPlaylistId() const;

	void SaveTabOrder() const;

	void RestoreTabOrder();

	void SetTabIcon(const QIcon &icon);

signals:
	void CreateNewPlaylist();

private:
	void CloseTab(int32_t tab_index);

	bool RemovePlaylist(int32_t playlist_id);

	void mouseDoubleClickEvent(QMouseEvent* e) override;
};

