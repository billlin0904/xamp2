#include <widget/cdpage.h>
#include <thememanager.h>
#include <widget/playlisttableview.h>
#include <widget/fonticon.h>
#include <widget/playlistpage.h>
#include <widget/util/str_util.h>
#include <ui_cdpage.h>

CdPage::CdPage(QWidget* parent)
    : QFrame(parent) {
    ui_ = new Ui::CDPage();
    ui_->setupUi(this);

    ui_->pcButton->setStyleSheet(qTEXT(R"(
                                        QToolButton#pcButton {
                                        border: none;
                                        background-color: transparent;
                                        }
                                        )"));
    ui_->pcButton->setIcon(qTheme.fontIcon(Glyphs::ICON_DESKTOP));
    ui_->pcButton->setIconSize(QSize(64, 64));

    ui_->cdButton->setStyleSheet(qTEXT(R"(
                                        QToolButton#cdButton {
                                        border: none;                                        
                                        background-color: transparent;
                                        }
                                        )"));
    ui_->cdButton->setIcon(qTheme.fontIcon(Glyphs::ICON_CD));
    ui_->cdButton->setIconSize(QSize(64, 64));

    ui_->arrowButton->setStyleSheet(qTEXT(R"(
                                        QToolButton#arrowButton {
                                        border: none;
                                        background-color: transparent;
                                        }
                                        )"));
    ui_->arrowButton->setIcon(qTheme.fontIcon(Glyphs::ICON_LEFT_ARROW));
    ui_->arrowButton->setIconSize(QSize(64, 64));
    ui_->playlistPage->hide();

    ui_->tipFrame->setStyleSheet(qTEXT("background-color: transparent;"));
    setStyleSheet(qTEXT("QFrame#CDPage { background-color: transparent; }"));
}

CdPage::~CdPage() {
    delete ui_;
}

void CdPage::onThemeChangedFinished(ThemeColor theme_color) {
    ui_->pcButton->setIcon(qTheme.fontIcon(Glyphs::ICON_DESKTOP));
    ui_->arrowButton->setIcon(qTheme.fontIcon(Glyphs::ICON_LEFT_ARROW));
    ui_->cdButton->setIcon(qTheme.fontIcon(Glyphs::ICON_CD));
}

PlaylistPage* CdPage::playlistPage() const {
    return ui_->playlistPage;
}

void CdPage::onRetranslateUi() {
	ui_->retranslateUi(this);
}

void CdPage::showPlaylistPage(bool show) {
    if (show) {
        ui_->tipFrame->hide();
        ui_->playlistPage->show();
    } else {
        ui_->tipFrame->show();
        ui_->playlistPage->hide();
    }
}
