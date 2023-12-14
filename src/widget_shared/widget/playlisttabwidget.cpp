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

PlaylistTabWidget::PlaylistTabWidget(QWidget* parent)
    : QTabWidget(parent) {
    setObjectName("playlistTab");
    setTabsClosable(true);
    setMouseTracking(true);
    setMovable(true);
    setAttribute(Qt::WA_StyledBackground);   

    switch (qTheme.themeColor()) {
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

    auto* tab_bar = new PlaylistTabBar(this);
    setTabBar(tab_bar);

    setContextMenuPolicy(Qt::CustomContextMenu);
    (void)QObject::connect(this, &PlaylistTabBar::customContextMenuRequested, [this](auto pt) {
        ActionMap<PlaylistTabWidget> action_map(this);

        auto* close_all_tab_act = action_map.addAction(tr("Close all tab"), [this]() {
            QList<PlaylistPage*> playlist_pages;
            for (auto i = 0; i < count(); ++i) {
                auto* playlist_page = dynamic_cast<PlaylistPage*>(widget(i));
                playlist_pages.append(playlist_page);
            }

            Q_FOREACH(auto * page, playlist_pages) {
                removePlaylist(page->playlist()->playlistId());
                page->deleteLater();
            }

            clear();
            emit removeAllPlaylist();
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
        try {
            action_map.exec(pt);
        }
        catch (Exception const& e) {
        }
        catch (std::exception const& e) {
        }
        });

    (void)QObject::connect(tab_bar, &PlaylistTabBar::textChanged, [this](auto index, const auto& name) {
        qMainDb.setPlaylistName(currentPlaylistId(), name);
        });

    (void)QObject::connect(this, &QTabWidget::tabCloseRequested,
        [this](auto tab_index) {
        if (XMessageBox::showYesOrNo(tr("Do you want to close tab ?")) == QDialogButtonBox::No) {
            return;
        }
        closeTab(tab_index);
        });
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
        qMainDb.setPlaylistIndex(playlist->playlistId(), i);
    }
}

void PlaylistTabWidget::restoreTabOrder() {
	const auto playlist_index = qMainDb.getPlaylistIndex();

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
    auto tab_index = currentIndex();
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

void PlaylistTabWidget::mouseDoubleClickEvent(QMouseEvent* e) {
    emit createNewPlaylist();
}
