#include <QTabBar>

#include <widget/str_utilts.h>
#include <widget/playlisttabwidget.h>

PlaylistTabWidget::PlaylistTabWidget(QWidget* parent)
	: QTabWidget(parent) {
    setObjectName("playlistTab");
    setStyleSheet(qTEXT("QTabWidget::pane { border: 0; }"));
    setTabsClosable(true);
    setMouseTracking(true);
    setMovable(false);

    (void)QObject::connect(tabBar(), &QTabBar::tabBarDoubleClicked,
        [this](auto tab_index) {
        });
}

void PlaylistTabWidget::mouseDoubleClickEvent(QMouseEvent* e) {
    emit CreateNewPlaylist();
}
