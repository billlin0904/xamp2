#include <QTabBar>
#include <QMouseEvent>

#include <widget/xmessagebox.h>
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

    auto* tab_bar = new PlaylistTabBar(this);
    setTabBar(tab_bar);

    (void)QObject::connect(tab_bar, &PlaylistTabBar::TextChanged, [this](auto index, const auto &name) {
        qMainDb.SetPlaylistName(GetCurrentPlaylistId(), name);
    });

    (void)QObject::connect(this, &QTabWidget::tabCloseRequested,
        [this](auto tab_index) {
        if (XMessageBox::ShowYesOrNo(tr("Do you want to close tab ?")) == QDialogButtonBox::No) {
            return;
        }
        auto* playlist_page = dynamic_cast<PlaylistPage*>(widget(tab_index));
        const auto* playlist = playlist_page->playlist();
        removeTab(tab_index);
        if (!qMainDb.transaction()) {
            return;
        }
        try {
            qMainDb.RemovePendingListMusic(playlist->GetPlaylistId());
            qMainDb.RemovePlaylistAllMusic(playlist->GetPlaylistId());
            qMainDb.RemovePlaylist(playlist->GetPlaylistId());
            qMainDb.commit();
            playlist_page->deleteLater();
        } catch (...) {
            qMainDb.rollback();
        }
        });
}

int32_t PlaylistTabWidget::GetCurrentPlaylistId() const {
    return dynamic_cast<PlaylistPage*>(currentWidget())->playlist()->GetPlaylistId();
}

void PlaylistTabWidget::mouseDoubleClickEvent(QMouseEvent* e) {
    emit CreateNewPlaylist();
}
