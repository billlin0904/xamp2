//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QTabWidget>
#include <QTabBar>

#include <widget/database.h>
#include <widget/themecolor.h>
#include <widget/widget_shared_global.h>
#include <widget/playlistpage.h>

class QMouseEvent;
class QPushButton;
class PlaylistTabBar;
class XTooltip;

class XAMP_WIDGET_SHARED_EXPORT PlaylistTabWidget final : public QTabWidget {
	Q_OBJECT
public:
	static constexpr QSize kTabIconSize = QSize(32, 32);
	
	explicit PlaylistTabWidget(QWidget* parent = nullptr);

	void createNewTab(const QString& name, QWidget* widget, bool resize = false);

	void hidePlusButton();

	int32_t currentPlaylistId() const;

	void setCurrentTabIndex(int32_t playlist_id);

	void saveTabOrder() const;

	void restoreTabOrder();

	void resetAllTabIcon();

	void setPlaylistTabIcon(const QIcon &icon);

	void setPlaylistCover(const QPixmap &cover);

	void closeAllTab();

	void setStoreType(StoreType type);

	void reloadAll();

	template <typename F>
	void forEachPlaylist(F &&func) {
		for (int i = 0; i < tabBar()->count(); ++i) {
			auto* playlist_page = dynamic_cast<PlaylistPage*>(widget(i));
			Q_ASSERT(playlist_page != nullptr);
			auto* playlist = playlist_page->playlist();
			func(playlist);
		}
	}

	PlaylistPage* findPlaylistPage(int32_t playlist_id);

	PlaylistPage* playlistPage(int32_t index) const;

	void setCurrentNowPlaying();

	void setNowPlaying(int32_t playlist_id);

	void setPlayerStateIcon(int32_t playlist_id, PlayerState state);
	
signals:
	void createNewPlaylist();

	void createCloudPlaylist();

	void removeAllPlaylist();

	void reloadAllPlaylist();

	void reloadPlaylist(int32_t tab_index);

	void deletePlaylist(const QString& playlist_id);

	void saveToM3UFile(int32_t playlist_id, const QString& playlist_name);

    void loadPlaylistFile(int32_t playlist_id);

public slots:
	void onThemeChangedFinished(ThemeColor theme_color);

	void onRetranslateUi();

private:
	void toolTipMove(const QPoint& pos);

	void mouseMoveEvent(QMouseEvent* event) override;

	void mousePressEvent(QMouseEvent* event) override;

	void leaveEvent(QEvent* event) override;

	void closeTab(int32_t tab_index);

	bool removePlaylist(int32_t playlist_id);

	void mouseDoubleClickEvent(QMouseEvent* e) override;

	void resizeEvent(QResizeEvent* event) override;

	bool eventFilter(QObject* watched, QEvent* event) override;

	void resizeTabWidth();

	void updateTabCloseButtons();

	int32_t hovered_tab_index_{ -1 };
	QTimer* tooltip_timer_{ nullptr };
	StoreType store_type_{ StoreType::PLAYLIST_LOCAL_STORE };
	QPushButton* add_tab_button_{ nullptr };
	PlaylistTabBar* tab_bar_{ nullptr };
	XTooltip* tooltip_{ nullptr };
	QElapsedTimer last_click_time_;
};

class XAMP_WIDGET_SHARED_EXPORT PlaylistTabPage : public QFrame {
	Q_OBJECT
public:
	explicit PlaylistTabPage(QWidget* parent);

	PlaylistTabWidget* tabWidget() const;
private:
	PlaylistTabWidget* tab_widget_{ nullptr };
};
