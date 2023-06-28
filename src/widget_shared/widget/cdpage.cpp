#include <widget/cdpage.h>
#include <thememanager.h>
#include <widget/playlisttableview.h>
#include <widget/fonticon.h>
#include <widget/playlistpage.h>
#include <widget/str_utilts.h>

CdPage::CdPage(QWidget* parent)
    : QFrame(parent) {
    ui_.setupUi(this);

    ui_.pcButton->setStyleSheet(qTEXT(R"(
                                        QToolButton#pcButton {
                                        border: none;
                                        background-color: transparent;
                                        }
                                        )"));
    ui_.pcButton->setIcon(qTheme.GetFontIcon(Glyphs::ICON_DESKTOP));
    ui_.pcButton->setIconSize(QSize(64, 64));

    ui_.cdButton->setStyleSheet(qTEXT(R"(
                                        QToolButton#cdButton {
                                        border: none;                                        
                                        background-color: transparent;
                                        }
                                        )"));
    ui_.cdButton->setIcon(qTheme.GetFontIcon(Glyphs::ICON_CD));
    ui_.cdButton->setIconSize(QSize(64, 64));

    ui_.arrowButton->setStyleSheet(qTEXT(R"(
                                        QToolButton#arrowButton {
                                        border: none;
                                        background-color: transparent;
                                        }
                                        )"));
    ui_.arrowButton->setIcon(qTheme.GetFontIcon(Glyphs::ICON_LEFT_ARROW));
    ui_.arrowButton->setIconSize(QSize(64, 64));

    ui_.playlistPage->playlist()->SetPodcastMode(false);
    ui_.playlistPage->hide();

    ui_.tipFrame->setStyleSheet(qTEXT("background-color: transparent;"));
    setStyleSheet(qTEXT("QFrame#CDPage { background-color: transparent; }"));
}

void CdPage::OnCurrentThemeChanged(ThemeColor theme_color) {
    ui_.pcButton->setIcon(qTheme.GetFontIcon(Glyphs::ICON_DESKTOP));
    ui_.arrowButton->setIcon(qTheme.GetFontIcon(Glyphs::ICON_LEFT_ARROW));
    ui_.cdButton->setIcon(qTheme.GetFontIcon(Glyphs::ICON_CD));
}

void CdPage::showPlaylistPage(bool show) {
    if (show) {
        ui_.tipFrame->hide();
        ui_.playlistPage->show();
    } else {
        ui_.tipFrame->show();
        ui_.playlistPage->hide();
    }
}
