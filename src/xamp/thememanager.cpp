#include "ui_xamp.h"
#include "widget/str_utilts.h"
#include "thememanager.h"

namespace ThemeManager {

void setNightStyle(Ui::XampWindow &ui) {
}

void setDefaultStyle(Ui::XampWindow &ui) {
    ui.currentView->setStyleSheet(Q_UTF8("background-color: rgba(228, 233, 237, 230);"));
    ui.titleFrame->setStyleSheet(Q_UTF8("background-color: rgba(228, 233, 237, 230);"));
    ui.sliderBar->setStyleSheet(Q_UTF8("background-color: rgba(228, 233, 237, 150);"));
    ui.controlFrame->setStyleSheet(Q_UTF8("background-color: rgba(255, 255, 255, 200);"));
    ui.volumeFrame->setStyleSheet(Q_UTF8("background-color: rgba(255, 255, 255, 200);"));
    ui.playingFrame->setStyleSheet(Q_UTF8("background-color: rgba(228, 233, 237, 220);"));
    ui.searchLineEdit->setStyleSheet(Q_UTF8(""));
	ui.sliderFrame->setStyleSheet(Q_UTF8("background-color: rgba(228, 233, 237, 230);"));
	ui.sliderBar->setStyleSheet(Q_UTF8("background-color: transparent;"));

    ui.closeButton->setStyleSheet(Q_UTF8(R"(
                                         QToolButton#closeButton {
                                         image: url(:/xamp/Resource/White/close.png);
                                         background-color: transparent;
                                         }
                                         )"));

    ui.minWinButton->setStyleSheet(Q_UTF8(R"(
                                          QToolButton#minWinButton {
                                          image: url(:/xamp/Resource/White/minimize.png);
                                          background-color: transparent;
                                          }
                                          )"));

    ui.maxWinButton->setStyleSheet(Q_UTF8(R"(
                                          QToolButton#maxWinButton {
                                          image: url(:/xamp/Resource/White/maximize.png);
                                          background-color: transparent;
                                          }
                                          )"));

    ui.settingsButton->setStyleSheet(Q_UTF8(R"(
                                            QToolButton#settingsButton {
                                            image: url(:/xamp/Resource/White/settings.png);
                                            background-color: transparent;
                                            }
                                            )"));

    ui.nextButton->setStyleSheet(Q_UTF8(R"(
                                        QToolButton#nextButton {
                                        border: none;
                                        image: url(:/xamp/Resource/White/next.png);
                                        background-color: transparent;
                                        }
                                        )"));

    ui.prevButton->setStyleSheet(Q_UTF8(R"(
                                        QToolButton#prevButton {
                                        border: none;
                                        image: url(:/xamp/Resource/White/previous.png);
                                        background-color: transparent;
                                        }
                                        )"));

    ui.nextPageButton->setStyleSheet(Q_UTF8(R"(
                                            QToolButton#nextPageButton {
                                            border: none;
                                            image: url(:/xamp/Resource/White/right_black.png);
                                            background-color: transparent;
                                            }
                                            )"));

    ui.backPageButton->setStyleSheet(Q_UTF8(R"(
                                            QToolButton#backPageButton {
                                            border: none;
                                            image: url(:/xamp/Resource/White/left_black.png);
                                            background-color: transparent;
                                            }
                                            )"));

    ui.searchLineEdit->setStyleSheet(Q_UTF8(R"(
                                            QLineEdit#searchLineEdit {
                                            background-color: white;
                                            border: none;
                                            }
                                            )"));

    ui.mutedButton->setStyleSheet(Q_UTF8(R"(
                                         QToolButton#mutedButton {
                                         image: url(:/xamp/Resource/White/volume_up.png);
                                         border: none;
                                         background-color: transparent;
                                         }
                                         )"));

    ui.selectDeviceButton->setStyleSheet(Q_UTF8(R"(
                                                QToolButton#selectDeviceButton {
                                                image: url(:/xamp/Resource/White/speaker.png);
                                                border: none;
                                                background-color: transparent;
                                                }
                                                )"));

    ui.playlistButton->setStyleSheet(Q_UTF8(R"(
                                            QToolButton#playlistButton {
                                            border: none;
                                            background-color: transparent;
                                            }
                                            )"));
    ui.volumeSlider->setStyleSheet(Q_UTF8(R"(
                                          QSlider#volumeSlider {
                                          border: none;
                                          background-color: transparent;
                                          }
                                          )"));

    ui.addPlaylistButton->setStyleSheet(Q_UTF8(R"(
                                               QToolButton#addPlaylistButton {
                                               border: none;
                                               background-color: transparent;
                                               }
                                               )"));

    ui.repeatButton->setStyleSheet(Q_UTF8(R"(
                                          QToolButton#repeatButton {
                                          image: url(:/xamp/Resource/White/repeat.png);
                                          background-color: transparent;
                                          border: none;
                                          }
                                          )"));

    ui.addPlaylistButton->setStyleSheet(Q_UTF8(R"(
                                               QToolButton#addPlaylistButton {
                                               image: url(:/xamp/Resource/White/create_new_folder.png);
                                               border: none;
                                               background-color: transparent;
                                               }
                                               )"));

    ui.titleLabel->setStyleSheet(Q_UTF8(R"(
                                        QLabel#titleLabel {
                                        background-color: transparent;
                                        }
                                        )"));

    ui.artistLabel->setStyleSheet(Q_UTF8(R"(
                                         QLabel#artistLabel {
                                         }
                                         )"));

    ui.startPosLabel->setStyleSheet(Q_UTF8(R"(
                                           QLabel#startPosLabel {
                                           color: gray;
                                           background-color: transparent;
                                           }
                                           )"));

    ui.endPosLabel->setStyleSheet(Q_UTF8(R"(
                                         QLabel#endPosLabel {
                                         color: gray;
                                         background-color: transparent;
                                         }
                                         )"));

    ui.seekSlider->setStyleSheet(Q_UTF8(R"(
                                        QSlider#seekSlider {
                                        border: none;
                                        background-color: transparent;
                                        }
                                        )"));

    ui.artistLabel->setStyleSheet(Q_UTF8(R"(
                                         QLabel#artistLabel {
                                         color: gray;
                                         background-color: transparent;
                                         }
                                         )"));
}
}
