#include "ui_xamp.h"
#include <widget/str_utilts.h>
#include <widget/image_utiltis.h>
#if defined(Q_OS_WIN)
#include <widget/win32/fluentstyle.h>
#endif
#include <widget/appsettings.h>
#include "thememanager.h"

QColor ThemeManager::tableTextColor(Qt::black);
QColor ThemeManager::backgroundColor(228, 233, 237, 230);
QColor ThemeManager::controlBackgroundColor(228, 233, 237, 220);
QSize ThemeManager::defaultAlbumCoverSize(250, 250);
QSize ThemeManager::defaultCoverSize(150, 150);
QColor ThemeManager::menuColor(228, 233, 237, 150);
QColor ThemeManager::menuTextColor(Qt::black);

DefaultStylePixmapManager::DefaultStylePixmapManager()
    : unknown_cover_(Q_UTF8(":/xamp/Resource/White/unknown_album.png"))
    , default_size_unknown_cover_(Pixmap::resizeImage(unknown_cover_, ThemeManager::getDefaultCoverSize()))
    , volume_up_(Q_UTF8(":/xamp/Resource/White/volume_up.png"))
    , volume_off_(Q_UTF8(":/xamp/Resource/White/volume_off.png")) {
}

const QPixmap& DefaultStylePixmapManager::defaultSizeUnknownCover() const noexcept {
    return default_size_unknown_cover_;
}

const QPixmap& DefaultStylePixmapManager::unknownCover() const noexcept {
    return unknown_cover_;
}

const QIcon& DefaultStylePixmapManager::volumeUp() const noexcept {
    return volume_up_;
}

const QIcon& DefaultStylePixmapManager::volumeOff() const noexcept {
    return volume_off_;
}

const StylePixmapManager& ThemeManager::pixmap() noexcept {
    static const DefaultStylePixmapManager manager;
    return manager;
}

void ThemeManager::setPlayOrPauseButton(Ui::XampWindow &ui, bool is_playing) {
    if (is_playing) {
        ui.playButton->setStyleSheet(Q_UTF8(R"(
                                            QToolButton#playButton {
                                            image: url(:/xamp/Resource/White/pause.png);
                                            border: none;
                                            background-color: transparent;
                                            }
                                            )"));
    }
    else {
        ui.playButton->setStyleSheet(Q_UTF8(R"(
                                            QToolButton#playButton {
                                            image: url(:/xamp/Resource/White/play.png);
                                            border: none;
                                            background-color: transparent;
                                            }
                                            )"));
    }
}

QString ThemeManager::getMenuStyle() noexcept {
    return Q_UTF8(R"(
                  QMenu {
                  background-color: rgba(228, 233, 237, 150);
                  }
                  QMenu::item:selected {
                  background-color: black;
                  }
                  )");
}

QSize ThemeManager::getDefaultCoverSize() noexcept {
    return defaultCoverSize;
}

QSize ThemeManager::getCacheCoverSize() noexcept {
    return getDefaultCoverSize() * 2;
}

QSize ThemeManager::getAlbumCoverSize() noexcept {
    return defaultAlbumCoverSize;
}

QColor ThemeManager::getBackgroundColor() noexcept {
    return backgroundColor;
}

void ThemeManager::enableBlur(const QWidget* widget, bool enable) {
#if defined(Q_OS_WIN)
    FluentStyle::setBlurMaterial(widget, enable);
    AppSettings::setValue(APP_SETTING_ENABLE_BLUR, enable);
#endif
}

void ThemeManager::setBackgroundColor(Ui::XampWindow& ui, QColor color) {
    ui.currentView->setStyleSheet(backgroundColorToString(color));
    ui.titleFrame->setStyleSheet(backgroundColorToString(color));
    ui.sliderFrame->setStyleSheet(backgroundColorToString(color));
    AppSettings::setValue(APP_SETTING_BACKGROUND_COLOR, color);
    backgroundColor = color;
}

void ThemeManager::setWhiteIcon(Ui::XampWindow& ui) {
    auto color = Q_UTF8("Black");

    ui.nextPageButton->setStyleSheet(QString(Q_UTF8(R"(
                                            QToolButton#nextPageButton {
                                            border: none;
                                            image: url(:/xamp/Resource/%1/right_black.png);
                                            background-color: transparent;
                                            }
                                            )")).arg(color));

    ui.backPageButton->setStyleSheet(QString(Q_UTF8(R"(
                                            QToolButton#backPageButton {
                                            border: none;
                                            image: url(:/xamp/Resource/%1/left_black.png);
                                            background-color: transparent;
                                            }
                                            )")).arg(color));

    ui.closeButton->setStyleSheet(QString(Q_UTF8(R"(
                                         QToolButton#closeButton {
                                         image: url(:/xamp/Resource/%1/close.png);
                                         background-color: transparent;
                                         }
                                         )")).arg(color));

    ui.minWinButton->setStyleSheet(QString(Q_UTF8(R"(
                                          QToolButton#minWinButton {
                                          image: url(:/xamp/Resource/%1/minimize.png);
                                          background-color: transparent;
                                          }
                                          )")).arg(color));

    ui.maxWinButton->setStyleSheet(QString(Q_UTF8(R"(
                                          QToolButton#maxWinButton {
                                          image: url(:/xamp/Resource/%1/maximize.png);
                                          background-color: transparent;
                                          }
                                          )")).arg(color));

    ui.settingsButton->setStyleSheet(QString(Q_UTF8(R"(
                                            QToolButton#settingsButton {
                                            image: url(:/xamp/Resource/%1/settings.png);
                                            background-color: transparent;
                                            }
                                            )")).arg(color));
}

void ThemeManager::setDefaultStyle(Ui::XampWindow &ui) {
    if (!AppSettings::getValue(APP_SETTING_BACKGROUND_COLOR).toString().isEmpty()) {
        setBackgroundColor(ui, AppSettings::getValue(APP_SETTING_BACKGROUND_COLOR).toString());
    }
    else {
        setBackgroundColor(ui, backgroundColor);
    }

    ui.controlFrame->setStyleSheet(backgroundColorToString(controlBackgroundColor));
    ui.volumeFrame->setStyleSheet(backgroundColorToString(controlBackgroundColor));
    ui.playingFrame->setStyleSheet(backgroundColorToString(controlBackgroundColor));

    ui.searchLineEdit->setStyleSheet(Q_UTF8(""));    
    ui.sliderBar->setStyleSheet(Q_UTF8("background-color: transparent;"));

    setWhiteIcon(ui);

    ui.stopButton->setStyleSheet(Q_UTF8(R"(
                                         QToolButton#stopButton {
                                         image: url(:/xamp/Resource/White/stop.png);
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
