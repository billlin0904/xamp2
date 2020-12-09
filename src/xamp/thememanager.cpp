#include "ui_xamp.h"

#include <widget/image_utiltis.h>

#include <QScreen>
#include <QApplication>

#if defined(Q_OS_WIN)
#include <widget/win32/win32.h>
#endif

#include <widget/appsettings.h>
#include "thememanager.h"

ThemeManager& ThemeManager::instance() {
    static ThemeManager manager;
    return manager;
}

ThemeManager::ThemeManager() {
    const auto screen_size = qApp->primaryScreen()->size();

    if ((screen_size.width() <= 1920 || screen_size.width() <= 2560) && screen_size.height() <= 1080) {
        cover_size_ = QSize(110, 110);
    }
    else {
        cover_size_ = QSize(150, 150);
    }

    tableTextColor = QColor(Qt::black);
    background_color_ = QColor(228, 233, 237, 150);
    control_background_color_ = background_color_;
    album_cover_size_ = QSize(250, 250);
    menu_color_ = QColor(228, 233, 237, 150);
    menu_text_color_ = QColor(Qt::black);
    setThemeColor(ThemeColor::DARK_THEME);
}

void ThemeManager::setThemeColor(ThemeColor theme_color) {
    theme_color_ = theme_color;
    emit themeChanged(theme_color_);
}

QLatin1String ThemeManager::themeColorPath() const {
    if (theme_color_ == ThemeColor::DARK_THEME) {
        return Q_UTF8("Black");
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

QIcon ThemeManager::volumeUp() const {
    return QIcon(Q_STR(":/xamp/Resource/%1/volume_up.png").arg(themeColorPath()));
}

QIcon ThemeManager::volumeOff() const {
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

QSize ThemeManager::getDefaultCoverSize() const noexcept {
    return cover_size_;
}

QSize ThemeManager::getCacheCoverSize() const noexcept {
    return getDefaultCoverSize() * 2;
}

QSize ThemeManager::getAlbumCoverSize() const noexcept {
    return album_cover_size_;
}

QColor ThemeManager::getBackgroundColor() const noexcept {
    return background_color_;
}

void ThemeManager::setBackgroundColor(QWidget* widget) {
    widget->setStyleSheet(backgroundColorToString(AppSettings::getValueAsString(kAppSettingBackgroundColor)));
}

void ThemeManager::enableBlur(const QWidget* widget, bool enable) const {
#if defined(Q_OS_WIN)
    win32::setBlurMaterial(widget, enable);
    AppSettings::setValue(kAppSettingEnableBlur, enable);
#else
    (void)widget;
    (void)enable;
#endif
}

QIcon ThemeManager::appIcon() const {
    return QIcon(Q_UTF8(":/xamp/xamp.ico"));
}

void ThemeManager::setBackgroundColor(Ui::XampWindow& ui, QColor color) {
    color.setAlpha(150);
	
    ui.currentView->setStyleSheet(backgroundColorToString(color));
    ui.titleFrame->setStyleSheet(backgroundColorToString(color));

    auto bottom_color = color.lighter(30);
    if (AppSettings::contains(kAppSettingBottomColor)) {
        bottom_color = AppSettings::getValueAsString(kAppSettingBottomColor);
    }

    ui.playingFrame->setStyleSheet(backgroundColorToString(bottom_color));
    ui.volumeFrame->setStyleSheet(backgroundColorToString(bottom_color));
    ui.controlFrame->setStyleSheet(backgroundColorToString(bottom_color));

    QColor alphaColor = color;
    alphaColor.setAlpha(200);
    if (AppSettings::contains(kAppSettingAlphaColor)) {
        alphaColor = AppSettings::getValueAsString(kAppSettingAlphaColor);
    }

    ui.sliderFrame->setStyleSheet(backgroundColorToString(alphaColor));

    AppSettings::setValue(kAppSettingBackgroundColor, color);
    background_color_ = color;    
    setThemeIcon(ui);    
}

QIcon ThemeManager::playArrow() const noexcept {
    return QIcon(Q_STR(":/xamp/Resource/%1/play_arrow.png").arg(themeColorPath()));
}

void ThemeManager::setShufflePlayorder(Ui::XampWindow& ui) const {
    const auto style_sheet = Q_STR(R"(
    QToolButton#repeatButton {
    image: url(:/xamp/Resource/%1/shuffle.png);
    border: none;
    background: transparent;
    }
    )").arg(themeColorPath());
    ui.repeatButton->setStyleSheet(style_sheet);
}

void ThemeManager::setRepeatOnePlayOrder(Ui::XampWindow& ui) const {
    const auto style_sheet = Q_STR(R"(
    QToolButton#repeatButton {
    image: url(:/xamp/Resource/%1/repeat.png);
    border: none;
    background: transparent;
    }
    )").arg(themeColorPath());
    ui.repeatButton->setStyleSheet(style_sheet);
}

void ThemeManager::setRepeatOncePlayOrder(Ui::XampWindow& ui) const {
    const auto style_sheet = Q_STR(R"(
    QToolButton#repeatButton {
    image: url(:/xamp/Resource/%1/repeat_one.png);
    border: none;
    background: transparent;
    }
    )").arg(themeColorPath());
    ui.repeatButton->setStyleSheet(style_sheet);
}

void ThemeManager::setThemeIcon(Ui::XampWindow& ui) const {
    ui.titleLabel->setStyleSheet(Q_UTF8(R"(
                                         QLabel#titleLabel {
                                         color: white;
                                         background-color: transparent;
                                         }
                                         )"));

    ui.artistLabel->setStyleSheet(Q_UTF8(R"(
                                         QLabel#artistLabel {
                                         color: gray;
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
                                         border: none;
                                         image: url(:/xamp/Resource/%1/close.png);
                                         background-color: transparent;
                                         }
                                         )").arg(themeColorPath()));

    ui.minWinButton->setStyleSheet(Q_STR(R"(
                                          QToolButton#minWinButton {
                                          border: none;
                                          image: url(:/xamp/Resource/%1/minimize.png);
                                          background-color: transparent;
                                          }
                                          )").arg(themeColorPath()));

    ui.maxWinButton->setStyleSheet(Q_STR(R"(
                                          QToolButton#maxWinButton {
                                          border: none;
                                          image: url(:/xamp/Resource/%1/maximize.png);
                                          background-color: transparent;
                                          }
                                          )").arg(themeColorPath()));

    ui.settingsButton->setStyleSheet(Q_STR(R"(
                                            QToolButton#settingsButton {
                                            border: none;
                                            image: url(:/xamp/Resource/%1/settings.png);
                                            background-color: transparent;
                                            }
                                            QToolButton#settingsButton::menu-indicator {
                                            image: none;
                                            }
                                            )").arg(themeColorPath()));

    ui.stopButton->setStyleSheet(Q_STR(R"(
                                         QToolButton#stopButton {
                                         border: none;
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
    setThemeIcon(ui);

    ui.searchLineEdit->setStyleSheet(Q_UTF8(R"(
                                            QLineEdit#searchLineEdit {
                                            background-color: white;
                                            border: none;
                                            color: gray;
                                            border-radius: 10px;
                                            }
                                            )"));

    ui.gaplessPlayButton->setStyleSheet(Q_UTF8(R"(
                                         QToolButton#gaplessPlayButton {
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
    
    QIcon search_icon(Q_UTF8(":/xamp/Resource/White/search.png"));
    ui.searchLineEdit->setClearButtonEnabled(true);
    ui.searchLineEdit->addAction(search_icon, QLineEdit::LeadingPosition);
}
