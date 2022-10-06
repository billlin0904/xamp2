#include "cdpage.h"
#include "thememanager.h"
#include <widget/playlisttableview.h>
#include <widget/FontIcon.h>
#include <widget/playlistpage.h>
#include <widget/str_utilts.h>

CdPage::CdPage(QWidget* parent)
    : QFrame(parent) {
    ui.setupUi(this);

    ui.pcButton->setStyleSheet(Q_TEXT(R"(
                                        QToolButton#pcButton {
                                        border: none;
                                        background-color: transparent;
                                        }
                                        )"));
    ui.pcButton->setIcon(Q_FONT_ICON_CODE(0xe30a));
    ui.pcButton->setIconSize(QSize(64, 64));

    ui.cdButton->setStyleSheet(Q_TEXT(R"(
                                        QToolButton#cdButton {
                                        border: none;                                        
                                        background-color: transparent;
                                        }
                                        )"));
    ui.cdButton->setIcon(Q_FONT_ICON_CODE(0xe019));
    ui.cdButton->setIconSize(QSize(64, 64));

    ui.arrowButton->setStyleSheet(Q_TEXT(R"(
                                        QToolButton#arrowButton {
                                        border: none;
                                        background-color: transparent;
                                        }
                                        )"));
    ui.arrowButton->setIcon(Q_FONT_ICON_CODE(0xe317));
    ui.arrowButton->setIconSize(QSize(64, 64));

    ui.playlistPage->playlist()->setPodcastMode(false);
    ui.playlistPage->hide();

    setStyleSheet(Q_TEXT("QFrame#CDPage { background-color: transparent; border: none; }"));
}

void CdPage::showPlaylistPage(bool show) {
    if (show) {
        ui.tipFrame->hide();
        ui.playlistPage->show();
    } else {
        ui.tipFrame->show();
        ui.playlistPage->hide();
    }
}