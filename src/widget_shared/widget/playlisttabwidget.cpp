#include <QTabBar>

#include <widget/database.h>
#include <widget/str_utilts.h>
#include <widget/playlisttabbar.h>
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
        auto name = tabText(tab_index);
        auto itr = playlist_map_.find(name);
        if (itr != playlist_map_.end()) {
            ClosePlaylist(itr.value());
            removeTab(tab_index);
            widget_map_[name]->deleteLater();
            widget_map_.remove(name);
            qMainDb.RemovePendingListMusic(itr.value());
            qMainDb.RemovePlaylistAllMusic(itr.value());            
            qMainDb.RemoveTable(itr.value());
        }
        });

    (void)QObject::connect(tabBar(), &QTabBar::tabBarDoubleClicked,
        [this](auto tab_index) {
        });
}

void PlaylistTabWidget::AddTab(int32_t playlist_id, const QString& name, QWidget* widget, bool add_db) {
    addTab(widget, name);

    playlist_map_.insert(name, playlist_id);
    widget_map_.insert(name, widget);

    if (add_db) {
        auto index = count();
        qMainDb.AddTable(name, index, playlist_id);
    }
}

void PlaylistTabWidget::mouseDoubleClickEvent(QMouseEvent* e) {
    emit CreateNewPlaylist();
}
