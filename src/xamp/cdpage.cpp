#include "cdpage.h"
#include "thememanager.h"
#include <widget/playlisttableview.h>
#include <widget/playlistpage.h>
#include <widget/str_utilts.h>

CdPage::CdPage(QWidget* parent)
    : QFrame(parent) {
    ui.setupUi(this);

    ui.pcButton->setStyleSheet(Q_STR(R"(
                                        QToolButton#pcButton {
                                        border: none;
                                        image: url(:/xamp/Resource/%1/computer.png);
                                        background-color: transparent;
                                        }
                                        )").arg(qTheme.themeColorPath()));
    ui.cdButton->setStyleSheet(Q_STR(R"(
                                        QToolButton#cdButton {
                                        border: none;
                                        image: url(:/xamp/Resource/%1/cd.png);
                                        background-color: transparent;
                                        }
                                        )").arg(qTheme.themeColorPath()));
    ui.arrowButton->setStyleSheet(Q_STR(R"(
                                        QToolButton#arrowButton {
                                        border: none;
                                        image: url(:/xamp/Resource/%1/backspace.png);
                                        background-color: transparent;
                                        }
                                        )").arg(qTheme.themeColorPath()));
    ui.playlistPage->playlist()->setPodcastMode(false);
    ui.playlistPage->hide();
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