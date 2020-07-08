#include "ui_xamp.h"

#include <widget/image_utiltis.h>

#include <QScreen>
#include <QApplication>

#if defined(Q_OS_WIN)
#include <dwmapi.h>
#include <widget/win32/fluentstyle.h>
#endif

#include <widget/appsettings.h>
#include "thememanager.h"

ThemeManager& ThemeManager::instance() {
    static ThemeManager manager;
    return manager;
}

ThemeManager::ThemeManager() {
    const auto sceen_size = qApp->screens()[0]->size();

    if ((sceen_size.width() <= 1920 || sceen_size.width() <= 2560) && sceen_size.height() <= 1080) {
        cover_size_ = QSize(110, 110);
    }
    else {
        cover_size_ = QSize(150, 150);
    }

    tableTextColor = QColor(Qt::black);
    background_color_ = QColor(228, 233, 237, 230);
    control_background_color_ = QColor(228, 233, 237, 220);
    album_cover_size_ = QSize(250, 250);
    menu_color_ = QColor(228, 233, 237, 150);
    menu_text_color_ = QColor(Qt::black);
    setThemeColor(ThemeColor::DARK_THEME);
}

void ThemeManager::setThemeColor(ThemeColor theme_color) {
    theme_color_ = theme_color;
    emit themeChanged(theme_color_);
}

ConstLatin1String ThemeManager::themeColorPath() const {
    switch (theme_color_) {
    case ThemeColor::DARK_THEME:
        return Q_UTF8("Black");
    default:
        return Q_UTF8("White");
    }
    return Q_UTF8("White");
}

DefaultStylePixmapManager::DefaultStylePixmapManager()
    : unknown_cover_(Q_UTF8(":/xamp/Resource/White/unknown_album.png"))
    , default_size_unknown_cover_(Pixmap::resizeImage(unknown_cover_, ThemeManager::instance().getDefaultCoverSize())) {
}

const QPixmap& DefaultStylePixmapManager::defaultSizeUnknownCover() const noexcept {
    return default_size_unknown_cover_;
}

const QPixmap& DefaultStylePixmapManager::unknownCover() const noexcept {
    return unknown_cover_;
}

const StylePixmapManager& ThemeManager::pixmap() noexcept {
    static const DefaultStylePixmapManager manager;
    return manager;
}

QIcon ThemeManager::volumeUp() {
    return QIcon(Q_STR(":/xamp/Resource/%1/volume_up.png").arg(themeColorPath()));
}

QIcon ThemeManager::volumeOff() {
    return QIcon(Q_STR(":/xamp/Resource/%1/volume_off.png").arg(themeColorPath()));
}

void ThemeManager::setPlayOrPauseButton(Ui::XampWindow& ui, bool is_playing) {
    if (is_playing) {
        ui.playButton->setStyleSheet(Q_STR(R"(
                                            QToolButton#playButton {
                                            image: url(:/xamp/Resource/%1/pause.png);
                                            border: none;
                                            background-color: transparent;
                                            }
                                            )").arg(themeColorPath()));
    }
    else {
        ui.playButton->setStyleSheet(Q_STR(R"(
                                            QToolButton#playButton {
                                            image: url(:/xamp/Resource/%1/play.png);
                                            border: none;
                                            background-color: transparent;
                                            }
                                            )").arg(themeColorPath()));
    }
}

QString ThemeManager::getMenuStyle() noexcept {
    return QEmptyString;
}

QSize ThemeManager::getDefaultCoverSize() noexcept {
    return cover_size_;
}

QSize ThemeManager::getCacheCoverSize() noexcept {
    return getDefaultCoverSize() * 2;
}

QSize ThemeManager::getAlbumCoverSize() noexcept {
    return album_cover_size_;
}

QColor ThemeManager::getBackgroundColor() noexcept {
    return background_color_;
}

void ThemeManager::enableBlur(const QWidget* widget, bool enable) {
#if defined(Q_OS_WIN)
    FluentStyle::setBlurMaterial(widget, enable);
    AppSettings::setValue(kAppSettingEnableBlur, enable);
#else
    (void)widget;
    (void)enable;
#endif
}

void ThemeManager::setBackgroundColor(Ui::XampWindow& ui, QColor color) {    
    ui.currentView->setStyleSheet(backgroundColorToString(color));
    ui.titleFrame->setStyleSheet(backgroundColorToString(color));
    
    QColor bottomColor = color.lighter(30);
    ui.playingFrame->setStyleSheet(backgroundColorToString(bottomColor));
    ui.volumeFrame->setStyleSheet(backgroundColorToString(bottomColor));
    ui.controlFrame->setStyleSheet(backgroundColorToString(bottomColor));

    QColor alphaColor = color;
    alphaColor.setAlpha(200);
    ui.sliderFrame->setStyleSheet(backgroundColorToString(alphaColor));

    AppSettings::setValue(kAppSettingBackgroundColor, color);
    background_color_ = color;    
    setThemeColor(ui);    
}

QIcon ThemeManager::playArrow() noexcept {
    return QIcon(Q_STR(":/xamp/Resource/%1/play_arrow.png").arg(themeColorPath()));
}

void ThemeManager::setShufflePlayorder(Ui::XampWindow& ui) {
    ui.repeatButton->setStyleSheet(Q_STR(R"(
                                              QToolButton#repeatButton {
                                              image: url(:/xamp/Resource/%1/shuffle.png);
                                              background: transparent;
                                              }
                                              )").arg(themeColorPath()));
}

void ThemeManager::setRepeatOnePlayorder(Ui::XampWindow& ui) {
    ui.repeatButton->setStyleSheet(Q_STR(R"(
                                              QToolButton#repeatButton {
                                              image: url(:/xamp/Resource/%1/repeat_one.png);
                                              background: transparent;
                                              }
                                              )").arg(themeColorPath()));
}

void ThemeManager::setRepeatOncePlayorder(Ui::XampWindow& ui) {
    ui.repeatButton->setStyleSheet(Q_STR(R"(
                                              QToolButton#repeatButton {
                                              image: url(:/xamp/Resource/%1/repeat.png);
                                              background: transparent;
                                              }
                                              )").arg(themeColorPath()));
}

void ThemeManager::setThemeColor(Ui::XampWindow& ui) {
    ui.titleLabel->setStyleSheet(Q_UTF8(R"(
                                         QLabel#titleLabel {
                                         color: white;
                                         background-color: transparent;
                                         }
                                         )"));

    ui.nextPageButton->setStyleSheet(Q_STR(R"(
                                            QToolButton#nextPageButton {
                                            border: none;
                                            image: url(:/xamp/Resource/%1/right_black.png);
                                            background-color: transparent;
                                            }
                                            )").arg(themeColorPath()));

    ui.backPageButton->setStyleSheet(Q_STR(R"(
                                            QToolButton#backPageButton {
                                            border: none;
                                            image: url(:/xamp/Resource/%1/left_black.png);
                                            background-color: transparent;
                                            }
                                            )").arg(themeColorPath()));

    ui.closeButton->setStyleSheet(Q_STR(R"(
                                         QToolButton#closeButton {
                                         image: url(:/xamp/Resource/%1/close.png);
                                         background-color: transparent;
                                         }
                                         )").arg(themeColorPath()));

    ui.minWinButton->setStyleSheet(Q_STR(R"(
                                          QToolButton#minWinButton {
                                          image: url(:/xamp/Resource/%1/minimize.png);
                                          background-color: transparent;
                                          }
                                          )").arg(themeColorPath()));

    ui.maxWinButton->setStyleSheet(Q_STR(R"(
                                          QToolButton#maxWinButton {
                                          image: url(:/xamp/Resource/%1/maximize.png);
                                          background-color: transparent;
                                          }
                                          )").arg(themeColorPath()));

    ui.settingsButton->setStyleSheet(Q_STR(R"(
                                            QToolButton#settingsButton {
                                            image: url(:/xamp/Resource/%1/settings.png);
                                            background-color: transparent;
                                            }
                                            QToolButton#settingsButton::menu-indicator { image: none; }
                                            )").arg(themeColorPath()));

    ui.stopButton->setStyleSheet(Q_STR(R"(
                                         QToolButton#stopButton {
                                         image: url(:/xamp/Resource/%1/stop.png);
                                         background-color: transparent;
                                         }
                                         )").arg(themeColorPath()));

    ui.nextButton->setStyleSheet(Q_STR(R"(
                                        QToolButton#nextButton {
                                        border: none;
                                        image: url(:/xamp/Resource/%1/next.png);
                                        background-color: transparent;
                                        }
                                        )").arg(themeColorPath()));

    ui.prevButton->setStyleSheet(Q_STR(R"(
                                        QToolButton#prevButton {
                                        border: none;
                                        image: url(:/xamp/Resource/%1/previous.png);
                                        background-color: transparent;
                                        }
                                        )").arg(themeColorPath()));

    ui.selectDeviceButton->setStyleSheet(Q_STR(R"(
                                                QToolButton#selectDeviceButton {
                                                image: url(:/xamp/Resource/%1/speaker.png);
                                                border: none;
                                                background-color: transparent;                                                
                                                }
                                                QToolButton#selectDeviceButton::menu-indicator { image: none; }
                                                )").arg(themeColorPath()));

    ui.mutedButton->setStyleSheet(Q_STR(R"(
                                         QToolButton#mutedButton {
                                         image: url(:/xamp/Resource/%1/volume_up.png);
                                         border: none;
                                         background-color: transparent;
                                         }
                                         )").arg(themeColorPath()));

    ui.repeatButton->setStyleSheet(Q_STR(R"(
                                          QToolButton#repeatButton {
                                          image: url(:/xamp/Resource/%1/repeat.png);
                                          background-color: transparent;
                                          border: none;
                                          }
                                          )").arg(themeColorPath()));

    ui.addPlaylistButton->setStyleSheet(Q_STR(R"(
                                               QToolButton#addPlaylistButton {
                                               image: url(:/xamp/Resource/%1/create_new_folder.png);
                                               border: none;
                                               background-color: transparent;
                                               }
                                               )").arg(themeColorPath()));

    ui.eqButton->setStyleSheet(Q_STR(R"(
                                          QToolButton#eqButton {
                                          image: url(:/xamp/Resource/%1/equalizer.png);
                                          background-color: transparent;
                                          border: none;
                                          }
                                          )").arg(themeColorPath()));

}

void ThemeManager::setDefaultStyle(Ui::XampWindow& ui) {
    if (!AppSettings::getValueAsString(kAppSettingBackgroundColor).isEmpty()) {
        setBackgroundColor(ui, AppSettings::getValueAsString(kAppSettingBackgroundColor));
    }
    else {
        setBackgroundColor(ui, background_color_);
    }

    ui.controlFrame->setStyleSheet(backgroundColorToString(control_background_color_));
    ui.volumeFrame->setStyleSheet(backgroundColorToString(control_background_color_));
    ui.playingFrame->setStyleSheet(backgroundColorToString(control_background_color_));

    ui.searchLineEdit->setStyleSheet(Q_UTF8(""));
    ui.sliderBar->setStyleSheet(Q_UTF8("QListView#sliderBar { background-color: transparent; border: none; }"));
	setThemeColor(ui);

    ui.searchLineEdit->setStyleSheet(Q_UTF8(R"(
                                            QLineEdit#searchLineEdit {
                                            background-color: white;
                                            border: none;
                                            color: gray;
                                            border-radius: 10px;
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
    QIcon search_icon(Q_UTF8(":/xamp/Resource/White/search.png"));
    ui.searchLineEdit->setClearButtonEnabled(true);
    ui.searchLineEdit->addAction(search_icon, QLineEdit::LeadingPosition);
}
