//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QTabWidget>

#include <widget/database.h>
#include <widget/themecolor.h>
#include <widget/widget_shared_global.h>

class QMouseEvent;

class XAMP_WIDGET_SHARED_EXPORT PlaylistTabWidget : public QTabWidget {
	Q_OBJECT
public:
	static constexpr QSize kTabIconSize = QSize(32, 32);

	explicit PlaylistTabWidget(QWidget* parent = nullptr);

	int32_t currentPlaylistId() const;

	void saveTabOrder() const;

	void restoreTabOrder();

	void setPlaylistTabIcon(const QIcon &icon);

	void createNewTab(const QString& name, QWidget* widget);

	void setPlaylistCover(const QPixmap &cover);

	void closeAllTab();

	void setStoreType(StoreType type);

	void reloadAll();

signals:
	void createNewPlaylist();

	void createCloudPlaylist();

	void removeAllPlaylist();

	void reloadAllPlaylist();

	void reloadPlaylist(int32_t tab_index);

	void deletePlaylist(const QString& playlist_id);

public slots:
	void onThemeChangedFinished(ThemeColor theme_color);

private:
	void closeTab(int32_t tab_index);

	bool removePlaylist(int32_t playlist_id);

	void mouseDoubleClickEvent(QMouseEvent* e) override;

	StoreType store_type_{ StoreType::LOCAL_STORE };
};

