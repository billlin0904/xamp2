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

    switch (qTheme.GetThemeColor()) {
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

        auto* close_all_tab_act = action_map.AddAction(tr("Close all tab"), [this]() {
            QList<PlaylistPage*> playlist_pages;
            for (auto i = 0; i < count(); ++i) {
                auto* playlist_page = dynamic_cast<PlaylistPage*>(widget(i));
                playlist_pages.append(playlist_page);
            }

            Q_FOREACH(auto * page, playlist_pages) {
                RemovePlaylist(page->playlist()->GetPlaylistId());
                page->deleteLater();
            }

            clear();
            emit RemoveAllPlaylist();
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
            emit RemoveAllPlaylist();
            });
        try {
            action_map.exec(pt);
        }
        catch (Exception const& e) {
        }
        catch (std::exception const& e) {
        }
        });

    (void)QObject::connect(tab_bar, &PlaylistTabBar::TextChanged, [this](auto index, const auto& name) {
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

void PlaylistTabWidget::SaveTabOrder() const {
    for (int i = 0; i < count(); ++i) {
        auto* playlist_page = dynamic_cast<PlaylistPage*>(widget(i));
        const auto* playlist = playlist_page->playlist();
        qMainDb.SetPlaylistIndex(playlist->GetPlaylistId(), i);
    }
}

void PlaylistTabWidget::RestoreTabOrder() {
	const auto playlist_index = qMainDb.GetPlaylistIndex();

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
            if (playlist->GetPlaylistId() == playlist_id) {
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
        setTabIcon(i, qTheme.GetFontIcon(Glyphs::ICON_CIRCLE_NOTCH));
    }
}

void PlaylistTabWidget::SetTabIcon(const QIcon& icon) {
    auto tab_index = currentIndex();
    if (tab_index != -1) {
        setTabIcon(tab_index, icon);
        for (int i = 0; i < count(); ++i) {
            if (i == tab_index) {
                continue;
            }
            setTabIcon(i, qTheme.GetFontIcon(Glyphs::ICON_CIRCLE_NOTCH));
        }
    }
}

void PlaylistTabWidget::CreateNewTab(const QString& name, QWidget* widget) {
    const auto index = addTab(widget, name);
    setTabIcon(index, qTheme.GetFontIcon(Glyphs::ICON_CIRCLE_NOTCH));
    setCurrentIndex(index);
}

void PlaylistTabWidget::mouseDoubleClickEvent(QMouseEvent* e) {
    emit CreateNewPlaylist();
}
