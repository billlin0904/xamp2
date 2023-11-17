#include <QTabBar>

#include <widget/database.h>
#include <widget/str_utilts.h>
#include <widget/playlisttabbar.h>
#include <widget/playlistpage.h>
#include <widget/playlisttableview.h>
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

    //setTabBar(new PlaylistTabBar(this));

    (void)QObject::connect(this, &QTabWidget::tabCloseRequested,
        [this](auto tab_index) {
        auto* playlist_page = dynamic_cast<PlaylistPage*>(widget(tab_index));
        auto* playlist = playlist_page->playlist();
        ClosePlaylist(playlist->GetPlaylistId());
        removeTab(tab_index);
        qMainDb.RemovePendingListMusic(playlist->GetPlaylistId());
        qMainDb.RemovePlaylistAllMusic(playlist->GetPlaylistId());
        qMainDb.RemoveTable(playlist->GetPlaylistId());
        playlist_page->deleteLater();
        });

    (void)QObject::connect(tabBar(), &QTabBar::tabBarDoubleClicked,
        [this](auto tab_index) {
        });
}

int32_t PlaylistTabWidget::GetCurrentPlaylistId() const {
    return dynamic_cast<PlaylistPage*>(currentWidget())->playlist()->GetPlaylistId();
}

void PlaylistTabWidget::AddTab(int32_t playlist_id, const QString& name, QWidget* widget, bool add_db) {
    addTab(widget, name);

    if (add_db) {
        auto index = count();
        qMainDb.AddTable(name, index, playlist_id);
    }
}

void PlaylistTabWidget::mouseDoubleClickEvent(QMouseEvent* e) {
    emit CreateNewPlaylist();
}
