#include "ui_xamp.h"

#include <widget/image_utiltis.h>

#include <QScreen>
#include <QMenu>
#include <QApplication>

#if defined(Q_OS_WIN)
#include <widget/win32/win32.h>
#else
#include <widget/osx/osx.h>
#endif

#include <widget/appsettings.h>
#include "thememanager.h"

ThemeManager& ThemeManager::instance() {
    static ThemeManager manager;
    return manager;
}

ThemeManager::ThemeManager() {
    cover_size_ = QSize(210, 210);
    table_text_color_ = QColor(Qt::black);
    background_color_ = QColor(228, 233, 237);
    control_background_color_ = background_color_;
    album_cover_size_ = QSize(250, 250);
    menu_color_ = QColor(228, 233, 237);
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

QIcon ThemeManager::makeIcon(const QString &path) const {
    QIcon icon;
    icon.addPixmap(path.arg(themeColorPath()), QIcon::Normal);
    icon.addPixmap(path.arg(themeColorPath()), QIcon::Selected);
    return icon;
}

QIcon ThemeManager::playlistIcon() const {
    return makeIcon(Q_STR(":/xamp/Resource/%1/tab_playlists.png"));
}

QIcon ThemeManager::podcastIcon() const {
    return makeIcon(Q_STR(":/xamp/Resource/%1/podcast.png"));
}

QIcon ThemeManager::albumsIcon() const {
    return makeIcon(Q_STR(":/xamp/Resource/%1/tab_albums.png"));
}

QIcon ThemeManager::artistsIcon() const {
    return makeIcon(Q_STR(":/xamp/Resource/%1/tab_artists.png"));
}

QIcon ThemeManager::subtitleIcon() const {
    return makeIcon(Q_STR(":/xamp/Resource/%1/tab_subtitles.png"));
}

QIcon ThemeManager::preferenceIcon() const  {
    return makeIcon(Q_STR(":/xamp/Resource/%1/preference.png"));
}

QIcon ThemeManager::aboutIcon() const {
    return makeIcon(Q_STR(":/xamp/Resource/%1/help.png"));
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

void ThemeManager::setBackgroundColor(QWidget* widget, int32_t alpha) {
    QColor color = AppSettings::getValueAsString(kAppSettingBackgroundColor);
    if (alpha > 0) {
        color.setAlpha(alpha);
    }
    widget->setStyleSheet(backgroundColorToString(color));
}

void ThemeManager::enableBlur(const QWidget* widget, bool enable) const {
#if defined(Q_OS_WIN)
    win32::setBlurMaterial(widget, enable);
#else
    osx::setBlurMaterial(widget, enable);
#endif
    AppSettings::setValue(kAppSettingEnableBlur, enable);
}

QIcon ThemeManager::appIcon() const {
    return QIcon(Q_UTF8(":/xamp/xamp.ico"));
}

void ThemeManager::setBackgroundColor(Ui::XampWindow& ui, QColor color) {
    ui.currentView->setStyleSheet(backgroundColorToString(color));
    ui.titleFrame->setStyleSheet(backgroundColorToString(color));
    
    ui.playingFrame->setStyleSheet(backgroundColorToString(QColor(77, 77, 77)));
    ui.volumeFrame->setStyleSheet(backgroundColorToString(QColor(45,45,45)));
    ui.controlFrame->setStyleSheet(backgroundColorToString(QColor(45,45,45)));

    if (AppSettings::contains(kAppSettingEnableBlur)) {
        ui.sliderFrame->setStyleSheet(backgroundColorToString(QColor(37, 37, 39, 210)));
    } else {
        ui.sliderFrame->setStyleSheet(backgroundColorToString(QColor(37, 37, 39)));
    }

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

QString ThemeManager::getMenuStlye() const {
    QColor color(37, 37, 39);
    return Q_STR(R"(
        QMenu {
            background: %1;
            border-radius: 4px;
            border: none;
        }
		QMenu:item:selected {
            background-color: palette(highlight);
			color: white;
			border-radius: 4px;
        }
        )").arg(colorToString(color));
}

void ThemeManager::setMenuStlye(QMenu *menu) const {
    menu->setStyleSheet(getMenuStlye());
    menu->setWindowFlags(menu->windowFlags() | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    menu->setAttribute(Qt::WA_TranslucentBackground);
}

void ThemeManager::setThemeIcon(Ui::XampWindow& ui) const {
    ui.titleFrameLabel->setStyleSheet(Q_STR(R"(
    QLabel#titleFrameLabel {
    border: none;
    background: transparent;
    }
    )"));

    ui.titleLabel->setStyleSheet(Q_UTF8(R"(
                                         QLabel#titleLabel {
                                         color: white;
                                         background-color: transparent;
                                         }
                                         )"));

    ui.artistLabel->setStyleSheet(Q_UTF8(R"(
                                         QLabel#artistLabel {
                                         color: red;
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


    setMenuStlye(ui.settingsButton->menu());

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

    ui.sampleConverterButton->setStyleSheet(Q_UTF8(R"(
                                         QToolButton#sampleConverterButton {
										 border: none;
										 font-weight: bold;
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

    ui.sliderBar->setStyleSheet(Q_UTF8(R"(
	QListView#sliderBar::item {
		border: 0px;
		padding-left: 15px;
	}
	QListVieww#sliderBar::text {
		left: 15px;
	}
	)"));


    ui.searchLineEdit->setClearButtonEnabled(true);
    ui.searchLineEdit->addAction(QIcon(Q_UTF8(":/xamp/Resource/White/search.png")),
        QLineEdit::LeadingPosition);
}
