#include "cdpage.h"
#include "thememanager.h"
#include <widget/playlisttableview.h>
#include <widget/fonticon.h>
#include <widget/playlistpage.h>
#include <widget/str_utilts.h>

CdPage::CdPage(QWidget* parent)
    : QFrame(parent) {
    ui.setupUi(this);

    ui.pcButton->setStyleSheet(qTEXT(R"(
                                        QToolButton#pcButton {
                                        border: none;
                                        background-color: transparent;
                                        }
                                        )"));
    ui.pcButton->setIcon(qTheme.iconFromFont(Glyphs::ICON_DESKTOP));
    ui.pcButton->setIconSize(QSize(64, 64));

    ui.cdButton->setStyleSheet(qTEXT(R"(
                                        QToolButton#cdButton {
                                        border: none;                                        
                                        background-color: transparent;
                                        }
                                        )"));
    ui.cdButton->setIcon(qTheme.iconFromFont(Glyphs::ICON_CD));
    ui.cdButton->setIconSize(QSize(64, 64));

    ui.arrowButton->setStyleSheet(qTEXT(R"(
                                        QToolButton#arrowButton {
                                        border: none;
                                        background-color: transparent;
                                        }
                                        )"));
    ui.arrowButton->setIcon(qTheme.iconFromFont(Glyphs::ICON_LEFT_ARROW));
    ui.arrowButton->setIconSize(QSize(64, 64));

    ui.playlistPage->playlist()->setPodcastMode(false);
    ui.playlistPage->hide();

    ui.tipFrame->setStyleSheet(qTEXT("background-color: transparent;"));
    setStyleSheet(qTEXT("QFrame#CDPage { background-color: transparent; }"));
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
