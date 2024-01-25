#include <QTabBar>
#include <QMouseEvent>

#include <widget/xmessagebox.h>
#include <widget/database.h>
#include <widget/str_utilts.h>
#include <widget/playlisttabbar.h>
#include <widget/playlistpage.h>
#include <widget/playlisttableview.h>
#include <widget/actionmap.h>
#include <widget/playlisttabwidget.h>

void PlaylistTabWidget::closeAllTab() {
    QList<PlaylistPage*> playlist_pages;
    for (auto i = 0; i < count(); ++i) {
        auto* playlist_page = dynamic_cast<PlaylistPage*>(widget(i));
        playlist_pages.append(playlist_page);
    }

    Q_FOREACH(auto * page, playlist_pages) {
        removePlaylist(page->playlist()->playlistId());
        page->deleteLater();
    }

    qMainDb.forEachPlaylist([this](auto playlist_id,
        auto,
        auto,
        auto,
        auto) {
            if (playlist_id == kDefaultAlbumPlaylistId
                || playlist_id == kDefaultCdPlaylistId
                || playlist_id == kDefaultYtMusicPlaylistId) {
                return;
            }
            removePlaylist(playlist_id);
        });

    clear();
    emit removeAllPlaylist();
}

void PlaylistTabWidget::setStoreType(StoreType type) {
    store_type_ = type;
    setTabsClosable(true);
    setMovable(store_type_ == StoreType::CLOUD_SEARCH_STORE ? true : false);
}

void PlaylistTabWidget::reloadAll() {
    for (auto i = 0; i < count(); ++i) {
        auto* playlist_page = dynamic_cast<PlaylistPage*>(widget(i));
        playlist_page->playlist()->reload();
    }
}

PlaylistTabWidget::PlaylistTabWidget(QWidget* parent)
    : QTabWidget(parent) {
    setObjectName("playlistTab");
    setMouseTracking(true);
    setAttribute(Qt::WA_StyledBackground);   

    onThemeChangedFinished(qTheme.themeColor());

    auto* tab_bar = new PlaylistTabBar(this);
    setTabBar(tab_bar);

    setContextMenuPolicy(Qt::CustomContextMenu);
    (void)QObject::connect(this, &PlaylistTabBar::customContextMenuRequested, [this](auto pt) {
        ActionMap<PlaylistTabWidget> action_map(this);

        if (store_type_ == StoreType::CLOUD_STORE) {
            action_map.setCallback(action_map.addAction(qTR("Create a playlist")), [this]() {
                emit createCloudPlaylist();
                });

            auto* reload_the_tab_act = action_map.addAction(qTR("Reload all playlist"), [this]() {
                emit reloadAllPlaylist();
                });

            auto* reload_the_playlist_act = action_map.addAction(qTR("Reload the playlist"), [pt, this]() {
                auto tab_index = tabBar()->tabAt(pt);
                if (tab_index == -1) {
                    return;
                }
                emit reloadPlaylist(tab_index);
                });

            auto* delete_the_tab_act = action_map.addAction(qTR("Delete the playlist"), [pt, this]() {
                auto tab_index = tabBar()->tabAt(pt);
                if (tab_index == -1) {
                    return;
                }

                auto* playlist_page = dynamic_cast<PlaylistPage*>(widget(tab_index));
                if (!playlist_page->playlist()->cloudPlaylistId()) {
                    return;
                }

                emit deletePlaylist(playlist_page->playlist()->cloudPlaylistId().value());
                });
        } else {
            auto* close_all_tab_act = action_map.addAction(qTR("Close all tab"), [this]() {
                closeAllTab();
                });

            auto* close_other_tab_act = action_map.addAction(qTR("Close other tab"), [pt, this]() {
                auto index = tabBar()->tabAt(pt);
                if (index == -1) {
                    return;
                }

                const auto* exclude_page = widget(index);

                QList<QWidget*> playlist_pages;
                for (auto i = 0; i < count(); ++i) {
                    auto* playlist_page = widget(i);
                    playlist_pages.append(playlist_page);
                }

                Q_FOREACH(auto * playlist_page, playlist_pages) {
                    if (playlist_page == exclude_page) {
                        continue;
                    }
                    const auto tab_index = indexOf(playlist_page);
                    closeTab(tab_index);
                }
                emit removeAllPlaylist();
                });
        }

        TRY_LOG(
            action_map.exec(pt);
        )
        });

    (void)QObject::connect(tab_bar, &PlaylistTabBar::textChanged, [this](auto index, const auto& name) {
        qMainDb.setPlaylistName(currentPlaylistId(), name);
        });

    (void)QObject::connect(this, &QTabWidget::tabCloseRequested,
        [this](auto tab_index) {
        if (XMessageBox::showYesOrNo(qTR("Do you want to close tab ?")) == QDialogButtonBox::No) {
            return;
        }
        auto* playlist_page = dynamic_cast<PlaylistPage*>(widget(tab_index));
        QString playlist;
        if (playlist_page->playlist()->cloudPlaylistId()) {
            playlist = playlist_page->playlist()->cloudPlaylistId().value();
        }
        emit deletePlaylist(playlist);
        closeTab(tab_index);
        });
}

void PlaylistTabWidget::onThemeChangedFinished(ThemeColor theme_color) {
    switch (theme_color) {
    case ThemeColor::DARK_THEME:
        setStyleSheet(qSTR(R"(
    QTabWidget::pane { 
		border: 0; 
	}

	QTabWidget QTabBar::tab {
		max-width: 120px;
		min-width: 120px;
		min-height: 30px;
	}
    )"));
        break;
    case ThemeColor::LIGHT_THEME:
        setStyleSheet(qSTR(R"(
    QTabWidget::pane { 
		border: 0; 
	}

	QTabWidget QTabBar::tab {
		max-width: 120px;
		min-width: 120px;
		min-height: 30px;
		color: black;
	}
    )"));
        break;
    default:
        break;
    }
}

bool PlaylistTabWidget::removePlaylist(int32_t playlist_id) {
    if (!qMainDb.transaction()) {
        return false;
    }
    try {
        qMainDb.removePlaylistAllMusic(playlist_id);
        qMainDb.removePlaylist(playlist_id);
        qMainDb.commit();
        return true;
    }
    catch (...) {
        qMainDb.rollback();
    }
    return false;
}

void PlaylistTabWidget::closeTab(int32_t tab_index) {
    auto* playlist_page = dynamic_cast<PlaylistPage*>(widget(tab_index));
    const auto* playlist = playlist_page->playlist();
    if (removePlaylist(playlist->playlistId())) {
        removeTab(tab_index);
        playlist_page->deleteLater();
    }
}

int32_t PlaylistTabWidget::currentPlaylistId() const {
    return dynamic_cast<PlaylistPage*>(currentWidget())->playlist()->playlistId();
}

void PlaylistTabWidget::saveTabOrder() const {
    for (int i = 0; i < count(); ++i) {
        auto* playlist_page = dynamic_cast<PlaylistPage*>(widget(i));
        const auto* playlist = playlist_page->playlist();
        qMainDb.setPlaylistIndex(playlist->playlistId(), i, store_type_);
    }
}

void PlaylistTabWidget::restoreTabOrder() {
	const auto playlist_index = qMainDb.getPlaylistIndex(store_type_);

    QList<QWidget*> widgets;
    QList<QString> texts;

    for (int i = 0; i < count(); ++i) {
        auto itr = playlist_index.find(i);
        if (itr == playlist_index.end()) {
            continue;
        }
        const auto playlist_id = itr->second;
        for (int j = 0; j < count(); ++j) {
            auto* playlist_page = dynamic_cast<PlaylistPage*>(widget(i));
            const auto* playlist = playlist_page->playlist();
            if (playlist->playlistId() == playlist_id) {
                widgets.append(playlist_page);
                texts.append(tabText(i));
            }
        }
    }

    this->clear();

    auto i = 0;
    for (auto* widget : widgets) {
        addTab(widget, texts[i]);
        ++i;
    }
    for (i = 0; i < count(); ++i) {
        setTabIcon(i, qTheme.applicationIcon());        
    }
}

void PlaylistTabWidget::setPlaylistTabIcon(const QIcon& icon) {
	const auto tab_index = currentIndex();
    if (tab_index != -1) {
        setTabIcon(tab_index, icon);
        for (int i = 0; i < count(); ++i) {
            if (i == tab_index) {
                continue;
            }
            setTabIcon(i, qTheme.applicationIcon());            
        }
    }
}

void PlaylistTabWidget::createNewTab(const QString& name, QWidget* widget) {
    const auto index = addTab(widget, name);
    setTabIcon(index, qTheme.applicationIcon());
    setCurrentIndex(index);
}

void PlaylistTabWidget::setPlaylistCover(const QPixmap& cover) {
    for (int i = 0; i < count(); ++i) {
        auto* playlist_page = dynamic_cast<PlaylistPage*>(widget(i));
        playlist_page->setCover(&cover);
    }
}

void PlaylistTabWidget::mouseDoubleClickEvent(QMouseEvent* e) {
    emit createNewPlaylist();
}
