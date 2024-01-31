#include <QHBoxLayout>
#include <QLabel>
#include <QTabBar>
#include <QMouseEvent>
#include <QPushButton>

#include <widget/xmessagebox.h>
#include <widget/database.h>
#include <widget/util/str_utilts.h>
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
            if (playlist_id == kAlbumPlaylistId
                || playlist_id == kCdPlaylistId
                || playlist_id == kYtMusicSearchPlaylistId) {
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

    auto* tab_bar = new PlaylistTabBar(this);
    setTabBar(tab_bar);

    plus_button_ = new QPushButton(this);    
    plus_button_->setMaximumSize(32, 32);
    plus_button_->setMinimumSize(32, 32);
    plus_button_->setIcon(qTheme.fontIcon(Glyphs::ICON_ADD));
    plus_button_->setIconSize(QSize(18, 18));
    plus_button_->setObjectName(qTEXT("plusButton"));

    (void)QObject::connect(plus_button_, &QPushButton::clicked, [this]() {
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

        TRY_LOG(
            action_map.exec(pt);
        )
        });

    (void)QObject::connect(tab_bar, &PlaylistTabBar::tabBarClicked, [this](auto index) {
        auto* playlist_page = dynamic_cast<PlaylistPage*>(widget(index));
        playlist_page->playlist()->reload();
        });

    (void)QObject::connect(tab_bar, &PlaylistTabBar::textChanged, [this](auto index, const auto& name) {
        qMainDb.setPlaylistName(currentPlaylistId(), name);
        });

    (void)QObject::connect(this, &QTabWidget::tabCloseRequested,
        [this](auto tab_index) {
        if (XMessageBox::showYesOrNo(tr("Do you want to close tab ?")) == QDialogButtonBox::No) {
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

    onThemeChangedFinished(qTheme.themeColor());
}

void PlaylistTabWidget::hidePlusButton() {
    plus_button_->hide();
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
		max-width: 200px;
		min-width: 200px;
		min-height: 30px;
		background-color: #121212;
	}

	QTabWidget QTabBar::tab:selected {		
		background-color: #54687A;
	}
    )"));

    plus_button_->setStyleSheet(qSTR(R"(   
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
		max-width: 200px;
		min-width: 200px;
		min-height: 30px;
		color: black;
		background-color: #f9f9f9;
	}

	QTabWidget QTabBar::tab:selected {		
		background-color: #C9CDD0;
	}
    )"));

        plus_button_->setStyleSheet(qSTR(R"(   
	QPushButton#plusButton:hover {
		background-color: #e1e3e5;
		border-radius: 8px;
	}
    )"));
        break;
    default:
        break;
    }
    plus_button_->setIcon(qTheme.fontIcon(Glyphs::ICON_ADD));
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
        setTabIcon(i, qTheme.fontIcon(Glyphs::ICON_DRAFT));
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
            setTabIcon(i, qTheme.fontIcon(Glyphs::ICON_DRAFT));
        }
    }
}

void PlaylistTabWidget::createNewTab(const QString& name, QWidget* widget) {
    const auto index = addTab(widget, name);
    setTabIcon(index, qTheme.fontIcon(Glyphs::ICON_DRAFT));
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

bool PlaylistTabWidget::eventFilter(QObject* watched, QEvent* event) {
    if (tabBar() == watched && event->type() == QEvent::Resize) {
        if (!plus_button_->isHidden()) {
            auto r = tabBar()->geometry();
            auto h = r.height();
            plus_button_->setFixedSize((h - 1) * QSize(1, 1));
            plus_button_->move(r.right() + 3, 0);
        }
    }
    return QTabWidget::eventFilter(watched, event);
}
