#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>

#include <widget/appsettings.h>
#include <widget/appsettingnames.h>
#include <widget/xmessagebox.h>
#include <widget/dao/playlistdao.h>
#include <widget/dao/musicdao.h>

#include <widget/util/str_util.h>
#include <widget/playlisttabbar.h>
#include <widget/playlistpage.h>
#include <widget/playlisttableview.h>
#include <widget/actionmap.h>
#include <widget/playlisttabwidget.h>

void PlaylistTabWidget::closeAllTab() {
    QList<PlaylistPage*> playlist_pages;
    for (auto i = 0; i < tabBar()->count(); ++i) {
        auto* playlist_page = dynamic_cast<PlaylistPage*>(widget(i));
        Q_ASSERT(playlist_page != nullptr);
        playlist_pages.append(playlist_page);
    }

    Q_FOREACH(auto * page, playlist_pages) {
        removePlaylist(page->playlist()->playlistId());
        page->deleteLater();
    }

    playlist_dao_.forEachPlaylist([this](auto playlist_id,
        auto,
        auto store_type,
        auto,
        auto) {
            if (playlist_id == kAlbumPlaylistId
                || playlist_id == kCdPlaylistId
                || playlist_id == kYtMusicSearchPlaylistId) {
                return;
            }
            if (store_type != store_type_) {
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
        Q_ASSERT(playlist_page != nullptr);
        playlist_page->playlist()->reload();
    }
}

PlaylistPage* PlaylistTabWidget::findPlaylistPage(int32_t playlist_id) {
	for (auto i = 0; i < count(); ++i) {
		auto* playlist_page = dynamic_cast<PlaylistPage*>(widget(i));
		Q_ASSERT(playlist_page != nullptr);
		if (playlist_page->playlist()->playlistId() == playlist_id) {
			return playlist_page;
		}
	}
	return nullptr;
}

void PlaylistTabWidget::setCurrentNowPlaying() {
    setPlaylistTabIcon(qTheme.playlistPlayingIcon(PlaylistTabWidget::kTabIconSize, 1.0));
}

void PlaylistTabWidget::setNowPlaying(int32_t playlist_id) {
    auto icon = qTheme.playlistPlayingIcon(PlaylistTabWidget::kTabIconSize, 1.0);

    for (auto i = 0; i < count(); ++i) {
        auto* playlist_page = dynamic_cast<PlaylistPage*>(widget(i));
        if (playlist_page->playlist()->playlistId() == playlist_id) {
            setTabIcon(i, icon);
        }
        else {
            setTabIcon(i, qTheme.fontIcon(Glyphs::ICON_DRAFT));
        }
    }
}

void PlaylistTabWidget::setPlayerStateIcon(int32_t playlist_id, PlayerState state) {
    QIcon icon;

    switch (state) {
    case PlayerState::PLAYER_STATE_RUNNING:
        icon = qTheme.playlistPauseIcon(PlaylistTabWidget::kTabIconSize, 1.0);
        break;
    case PlayerState::PLAYER_STATE_PAUSED:
        icon = qTheme.playlistPlayingIcon(PlaylistTabWidget::kTabIconSize, 1.0);
        break;
    case PlayerState::PLAYER_STATE_STOPPED:
    case PlayerState::PLAYER_STATE_USER_STOPPED:
    default:
        icon = qTheme.fontIcon(Glyphs::ICON_DRAFT);
        break;
    }

    for (auto i = 0; i < count(); ++i) {
        auto* playlist_page = dynamic_cast<PlaylistPage*>(widget(i));
        if (playlist_page->playlist()->playlistId() == playlist_id) {
            setTabIcon(i, icon);
        }
        else {
            setTabIcon(i, qTheme.fontIcon(Glyphs::ICON_DRAFT));
        }
    }
}

PlaylistTabWidget::PlaylistTabWidget(QWidget* parent)
    : QTabWidget(parent) {
    setObjectName("playlistTab");
    setMouseTracking(true);
    setAttribute(Qt::WA_StyledBackground);

    auto* tab_bar = new PlaylistTabBar(this);
    setTabBar(tab_bar);

    add_tab_button_ = new QPushButton(this);    
    add_tab_button_->setMaximumSize(32, 32);
    add_tab_button_->setMinimumSize(32, 32);
    add_tab_button_->setIcon(qTheme.fontIcon(Glyphs::ICON_ADD));
    add_tab_button_->setIconSize(QSize(18, 18));
    add_tab_button_->setObjectName(qTEXT("plusButton"));

    (void)QObject::connect(add_tab_button_, &QPushButton::clicked, [this]() {
        emit createNewPlaylist();
        });
    tabBar()->installEventFilter(this);

    setContextMenuPolicy(Qt::CustomContextMenu);
    (void)QObject::connect(this, &PlaylistTabBar::customContextMenuRequested, [this](auto pt) {
        ActionMap<PlaylistTabWidget> action_map(this);

        if (store_type_ == StoreType::CLOUD_STORE) {
            action_map.setCallback(action_map.addAction(tr("Create a playlist")), [this]() {
                emit createCloudPlaylist();
                });

            auto* reload_the_tab_act = action_map.addAction(tr("Reload all playlist"), [this]() {
                emit reloadAllPlaylist();
                });

            auto* reload_the_playlist_act = action_map.addAction(tr("Reload the playlist"), [pt, this]() {
                auto tab_index = tabBar()->tabAt(pt);
                if (tab_index == -1) {
                    return;
                }
                emit reloadPlaylist(tab_index);
                });

            auto* delete_the_tab_act = action_map.addAction(tr("Delete the playlist"), [pt, this]() {
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
            auto* close_all_tab_act = action_map.addAction(tr("Close all tab"), [this]() {
                closeAllTab();
                });

            auto* close_other_tab_act = action_map.addAction(tr("Close other tab"), [pt, this]() {
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

        XAMP_TRY_LOG(
            action_map.exec(pt);
        );
        });

    (void)QObject::connect(tab_bar, &PlaylistTabBar::tabBarClicked, [this](auto index) {
        auto* playlist_page = dynamic_cast<PlaylistPage*>(widget(index));
        Q_ASSERT(playlist_page != nullptr);
        playlist_page->playlist()->reload();
        });

    (void)QObject::connect(tab_bar, &PlaylistTabBar::textChanged, [this](auto index, const auto& name) {
        playlist_dao_.setPlaylistName(currentPlaylistId(), name);
		qAppSettings.setValue(kAppSettingLastPlaylistTabIndex, currentPlaylistId());
        });

    (void)QObject::connect(tab_bar, &PlaylistTabBar::tabBarClicked, [this](auto index) {
        auto* playlist_page = dynamic_cast<PlaylistPage*>(widget(index));
        Q_ASSERT(playlist_page != nullptr);
        qAppSettings.setValue(kAppSettingLastPlaylistTabIndex, playlist_page->playlist()->playlistId());
        });

    (void)QObject::connect(this, &QTabWidget::tabCloseRequested,
        [this](auto tab_index) {
        /*if (XMessageBox::showYesOrNo(tr("Do you want to close tab ?")) == QDialogButtonBox::No) {
            return;
        }*/
        auto* playlist_page = dynamic_cast<PlaylistPage*>(widget(tab_index));
        Q_ASSERT(playlist_page != nullptr);
        QString playlist;
        if (playlist_page->playlist()->cloudPlaylistId()) {
            playlist = playlist_page->playlist()->cloudPlaylistId().value();
        }
        emit deletePlaylist(playlist);
        closeTab(tab_index);
        if (!count()) {
            emit removeAllPlaylist();
        }
        });

    onThemeChangedFinished(qTheme.themeColor());
}

void PlaylistTabWidget::hidePlusButton() {
    add_tab_button_->hide();
}

void PlaylistTabWidget::onRetranslateUi() {

}

void PlaylistTabWidget::onThemeChangedFinished(ThemeColor theme_color) {
    switch (theme_color) {
    case ThemeColor::DARK_THEME:
        setStyleSheet(qSTR(R"(
	QTabWidget {
		qproperty-iconSize: 16px 16px;	
	}

    QTabWidget::pane { 
		border: 0; 
	}

	QTabWidget QTabBar::tab {
		min-height: 30px;
		background-color: #121212;
	}

	QTabWidget QTabBar::tab:selected {		
		background-color: #54687A;
		font-weight: bold;
	}
    )"));

    add_tab_button_->setStyleSheet(qSTR(R"(   
	QPushButton#plusButton:hover {
		background-color: #455364;
		border-radius: 8px;
	}
    )"));
        break;
    case ThemeColor::LIGHT_THEME:
        setStyleSheet(qSTR(R"(
	QTabWidget {
		qproperty-iconSize: 16px 16px;	
	}

    QTabWidget::pane { 
		border: 0; 
	}

	QTabWidget QTabBar::tab {
		min-height: 30px;
		color: black;
		background-color: #f9f9f9;
	}

	QTabWidget QTabBar::tab:selected {		
		background-color: #C9CDD0;
		font-weight: bold;
	}
    )"));

        add_tab_button_->setStyleSheet(qSTR(R"(   
	QPushButton#plusButton:hover {
		background-color: #e1e3e5;
		border-radius: 8px;
	}
    )"));
        break;
    default:
        break;
    }
    add_tab_button_->setIcon(qTheme.fontIcon(Glyphs::ICON_ADD));

    for (int i = 0; i < tabBar()->count(); ++i) {
        setTabIcon(i, qTheme.fontIcon(Glyphs::ICON_DRAFT));
    }
}

bool PlaylistTabWidget::removePlaylist(int32_t playlist_id) {
    Transaction scope;
    dao::PlaylistDao playlist_dao(qGuiDb.getDatabase());

    return scope.complete([&]() {
        playlist_dao.removePlaylistAllMusic(playlist_id);
        playlist_dao.removePlaylist(playlist_id);
        });
}

void PlaylistTabWidget::closeTab(int32_t tab_index) {
    auto* playlist_page = dynamic_cast<PlaylistPage*>(widget(tab_index));
    Q_ASSERT(playlist_page != nullptr);
    const auto* playlist = playlist_page->playlist();
    if (removePlaylist(playlist->playlistId())) {
        removeTab(tab_index);
        playlist_page->deleteLater();
    }
}

void PlaylistTabWidget::createNewTab(const QString& name, QWidget* widget) {
    const auto index = addTab(widget, name);
    setTabIcon(index, qTheme.fontIcon(Glyphs::ICON_DRAFT));
    setCurrentIndex(index);
}

int32_t PlaylistTabWidget::currentPlaylistId() const {
    return dynamic_cast<PlaylistPage*>(currentWidget())->playlist()->playlistId();
}

void PlaylistTabWidget::setCurrentTabIndex(int32_t playlist_id) {
	for (auto i = 0; i < tabBar()->count(); ++i) {
		auto* playlist_page = dynamic_cast<PlaylistPage*>(widget(i));
		if (playlist_page->playlist()->playlistId() == playlist_id) {
			setCurrentIndex(i);
			break;
		}
	}
}

void PlaylistTabWidget::saveTabOrder() const {
    dao::PlaylistDao playlist_dao;

    for (int i = 0; i < tabBar()->count(); ++i) {
        auto* playlist_page = dynamic_cast<PlaylistPage*>(widget(i));
        Q_ASSERT(playlist_page != nullptr);
        const auto* playlist = playlist_page->playlist();
        playlist_dao.setPlaylistIndex(playlist->playlistId(), i, store_type_);
        XAMP_LOG_DEBUG("saveTabOrder: {} at {}", tabText(i).toStdString(), i);
    }
}

void PlaylistTabWidget::restoreTabOrder() {
    dao::PlaylistDao playlist_dao;
	const auto playlist_index = playlist_dao.getPlaylistIndex(store_type_);

    QList<QString> texts;
    QList<int> new_order;

    for (auto& [index, playlist_id] : playlist_index) {
        auto found = false;
        for (int j = 0; j < tabBar()->count(); ++j) {
            auto* playlist_page = dynamic_cast<PlaylistPage*>(widget(j));
            Q_ASSERT(playlist_page != nullptr);
            const auto widget_playlist_id = playlist_page->playlist()->playlistId();
            if (widget_playlist_id == playlist_id) {
                XAMP_LOG_DEBUG("restoreTabOrder: {} at {}", widget_playlist_id, j);
                texts.append(tabText(j));
                new_order.append(j);
                found = true;
                break;
            }
        }

        Q_ASSERT(found);
    }

    // 移動 tab 的順序
    for (int i = 0; i < new_order.size(); ++i) {
        tabBar()->moveTab(new_order[i], i);
    }

    // 重新設置 tab 的標籤
    for (int i = 0; i < new_order.size(); ++i) {
        setTabText(i, texts[i]);
    }

    // 重新設置 tab 的 icon
    for (int i = 0; i < new_order.size(); ++i) {
        setTabIcon(i, qTheme.fontIcon(Glyphs::ICON_DRAFT));
    }

    for (int i = 0; i < tabBar()->count(); ++i) {
        XAMP_LOG_DEBUG("restoreTabOrder: {} at {}", tabText(i).toStdString(), i);
    }
}

void PlaylistTabWidget::resetAllTabIcon() {
    for (int i = 0; i < tabBar()->count(); ++i) {
        setTabIcon(i, qTheme.fontIcon(Glyphs::ICON_DRAFT));
    }
}

void PlaylistTabWidget::setPlaylistTabIcon(const QIcon& icon) {
	const auto tab_index = currentIndex();
    if (tab_index != -1) {
        setTabIcon(tab_index, icon);
        for (int i = 0; i < tabBar()->count(); ++i) {
            if (i == tab_index) {
                continue;
            }
            setTabIcon(i, qTheme.fontIcon(Glyphs::ICON_DRAFT));
        }
    }
}

void PlaylistTabWidget::setPlaylistCover(const QPixmap& cover) {
    for (int i = 0; i < tabBar()->count(); ++i) {
        auto* playlist_page = dynamic_cast<PlaylistPage*>(widget(i));
        Q_ASSERT(playlist_page != nullptr);
        playlist_page->setCover(&cover);
    }
}

void PlaylistTabWidget::mouseDoubleClickEvent(QMouseEvent* e) {
    emit createNewPlaylist();
}

void PlaylistTabWidget::resizeEvent(QResizeEvent* event) {
    QTabWidget::resizeEvent(event);    
    if (tabBar()->count() > PlaylistTabBar::kSmallTabCount) {
        tabBar()->setFixedWidth(width() - PlaylistTabBar::kMaxButtonWidth);
    }    
}

bool PlaylistTabWidget::eventFilter(QObject* watched, QEvent* event) {
    if (tabBar() == watched) {
        if (event->type() == QEvent::Resize) {
            if (!add_tab_button_->isHidden()) {
                auto r = tabBar()->geometry();
                auto h = r.height();
                if (h > 0) {
                    add_tab_button_->setFixedSize((h - 1) * QSize(1, 1));
                    auto right = r.right() + 3;
                    add_tab_button_->move(right, 0);
                }
            }
        }
        else {
            if (!add_tab_button_->isHidden() && tabBar()->count() == 0) {
                auto r = tabBar()->geometry();
                
                add_tab_button_->setFixedSize((32 - 1) * QSize(1, 1));
                auto right = r.right() + 3;
                add_tab_button_->move(right, 0);
            }            
        }        
    }
    return QTabWidget::eventFilter(watched, event);
}
