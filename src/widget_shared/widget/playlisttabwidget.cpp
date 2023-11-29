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
    setMovable(false);
    setAttribute(Qt::WA_StyledBackground);
    setStyleSheet(qSTR(R"(
    QTabWidget::pane { 
		border: 0; 
	}

	QTabWidget::tab {
		background-color: black;
	}	
    )"));

    auto* tab_bar = new PlaylistTabBar(this);
    setTabBar(tab_bar);

    setContextMenuPolicy(Qt::CustomContextMenu);
    (void)QObject::connect(this, &PlaylistTabBar::customContextMenuRequested, [this](auto pt) {
        ActionMap<PlaylistTabWidget> action_map(this);

        auto* close_all_tab_act = action_map.AddAction(tr("Close all tab"), [this]() {
            QList<PlaylistPage*> playlist_pages;
            for (auto i = 0; i < count(); ++i) {
                auto* playlist_page = dynamic_cast<PlaylistPage*>(widget(i));
                playlist_pages.append(playlist_page);
            }

            Q_FOREACH(auto *page, playlist_pages) {
                RemovePlaylist(page->playlist()->GetPlaylistId());
                page->deleteLater();
            }

            clear();
            });

        auto* close_other_tab_act = action_map.AddAction(tr("Close other tab"), [pt, this]() {
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
                CloseTab(tab_index);
            }

            });
        try {
            action_map.exec(pt);
        }
        catch (Exception const& e) {
        }
        catch (std::exception const& e) {
        }
    });

    (void)QObject::connect(tab_bar, &PlaylistTabBar::TextChanged, [this](auto index, const auto &name) {
        qMainDb.SetPlaylistName(GetCurrentPlaylistId(), name);
    });

    (void)QObject::connect(this, &QTabWidget::tabCloseRequested,
        [this](auto tab_index) {
        if (XMessageBox::ShowYesOrNo(tr("Do you want to close tab ?")) == QDialogButtonBox::No) {
            return;
        }
        CloseTab(tab_index);
        });
}

bool PlaylistTabWidget::RemovePlaylist(int32_t playlist_id) {
    if (!qMainDb.transaction()) {
        return false;
    }
    try {
        qMainDb.RemovePlaylistAllMusic(playlist_id);
        qMainDb.RemovePlaylist(playlist_id);
        qMainDb.commit();
        return true;
    }
    catch (...) {
        qMainDb.rollback();
    }
    return false;
}

void PlaylistTabWidget::CloseTab(int32_t tab_index) {
    auto* playlist_page = dynamic_cast<PlaylistPage*>(widget(tab_index));
    const auto* playlist = playlist_page->playlist();
    if (RemovePlaylist(playlist->GetPlaylistId())) {
        removeTab(tab_index);
        playlist_page->deleteLater();
    }
}

int32_t PlaylistTabWidget::GetCurrentPlaylistId() const {
    return dynamic_cast<PlaylistPage*>(currentWidget())->playlist()->GetPlaylistId();
}

void PlaylistTabWidget::mouseDoubleClickEvent(QMouseEvent* e) {
    emit CreateNewPlaylist();
}
