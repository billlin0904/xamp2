#include "ui_xamp.h"

#include <widget/image_utiltis.h>
#include <QGraphicsDropShadowEffect>
#include <QScreen>
#include <QResource>
#include <QApplication>
#include <QFontDatabase>
#include <QTextStream>

#if defined(Q_OS_WIN)
#include <widget/win32/win32.h>
#else
#include <widget/osx/osx.h>
#endif

#include <widget/str_utilts.h>
#include <widget/appsettings.h>
#include "thememanager.h"

bool ThemeManager::useNativeWindow() const {
    return use_native_window_;
}

bool ThemeManager::enableBlur() const {
    return AppSettings::getValueAsBool(kAppSettingEnableBlur);
}

QFont ThemeManager::loadFonts() {
    const auto digital_font_id = QFontDatabase::addApplicationFont(Q_TEXT(":/xamp/fonts/Lato-Regular.ttf"));
    const auto digital_font_families = QFontDatabase::applicationFontFamilies(digital_font_id);

    QString default_font;
    QString default_bold_font;

    auto font_name = AppSettings::getValueAsString(kAppSettingDefaultFontName);
    if (font_name.isEmpty()) {
        font_name = Q_TEXT("WorkSans");
    }

    default_font = Q_STR(":/xamp/fonts/%1-Regular.ttf").arg(font_name);
    default_bold_font = Q_STR(":/xamp/fonts/%1-Bold.ttf").arg(font_name);

    const auto title_font_id = QFontDatabase::addApplicationFont(default_bold_font);
    auto title_font_families = QFontDatabase::applicationFontFamilies(title_font_id);

    const auto default_font_id = QFontDatabase::addApplicationFont(default_font);
    auto default_font_families = QFontDatabase::applicationFontFamilies(default_font_id);

    QList<QString> ui_fallback_fonts;

    ui_fallback_fonts.push_back(default_font_families[0]);
    ui_fallback_fonts.push_back(title_font_families[0]);
#if defined(Q_OS_WIN)
    // note: If we are support Source HanSans font sets must be enable Direct2D function,
    // But Qt framework not work fine with that!
    // -platform direct2d
    /*const auto source_han_sans_font_path =
        Q_STR("%1/fonts/SourceHanSans.ttc")
		.arg(QCoreApplication::applicationDirPath());
    const auto loaded_font_id = QFontDatabase::addApplicationFont(source_han_sans_font_path);
    if (loaded_font_id != -1) {
        auto loaded_font_families = QFontDatabase::applicationFontFamilies(loaded_font_id);
        Q_FOREACH(const auto font_name, loaded_font_families) {
            if (font_name.contains(Q_TEXT("HW")) 
                || font_name.contains(Q_TEXT("Light"))
                || font_name.contains(Q_TEXT("Medium"))
                || font_name.contains(Q_TEXT("Heavy"))) {
                continue;
            }
            ui_fallback_fonts.push_back(font_name);
        }
    }*/

    ui_fallback_fonts.push_back(Q_TEXT("Meiryo UI")); // For japanese font.
    ui_fallback_fonts.push_back(Q_TEXT("Microsoft YaHei"));
    ui_fallback_fonts.push_back(Q_TEXT("Microsoft JhengHei UI"));

    ui_fallback_fonts.push_back(Q_TEXT("Arial"));
    ui_fallback_fonts.push_back(Q_TEXT("Lucida Grande"));
    ui_fallback_fonts.push_back(Q_TEXT("Helvetica Neue"));
    QFont::insertSubstitutions(Q_TEXT("MonoFont"), QList<QString>() << Q_TEXT("Consolas"));
#else
    QList<QString> mono_fonts;
    auto family = QFontDatabase::systemFont(QFontDatabase::FixedFont).family();
    XAMP_LOG_DEBUG("MonoFont Family : {}", family.toStdString());
    mono_fonts.push_back(family);
    QFont::insertSubstitutions(Q_TEXT("MonoFont"), mono_fonts);
    ui_fallback_fonts.push_back(Q_TEXT("PingFang TC"));
    ui_fallback_fonts.push_back(Q_TEXT("Heiti TC"));
#endif
    QFont::insertSubstitutions(Q_TEXT("UI"), ui_fallback_fonts);
    QFont::insertSubstitutions(Q_TEXT("FormatFont"), digital_font_families);

    QFont ui_font(Q_TEXT("UI"));
    ui_font.setStyleStrategy(QFont::PreferAntialias);
    return ui_font;
}

void ThemeManager::setPalette() {
    palette_ = QPalette();
    if (theme_color_ == ThemeColor::LIGHT_THEME) {
        palette_.setColor(QPalette::WindowText, QColor(250, 250, 250));
        background_color_ = QColor(250, 250, 250);
    } else {
        palette_.setColor(QPalette::WindowText, QColor(25, 35, 45));
        background_color_ = QColor(25, 35, 45);
    }
}

ThemeManager::ThemeManager() {
    cover_size_ = QSize(210, 210);
    album_cover_size_ = QSize(250, 250);
    //setThemeColor(ThemeColor::DARK_THEME);
    //setThemeColor(ThemeColor::LIGHT_THEME);
    const auto theme = AppSettings::getAsEnum<ThemeColor>(kAppSettingTheme);
    setThemeColor(theme);
    ui_font_ = loadFonts();
    use_native_window_ = !AppSettings::getValueAsBool(kAppSettingUseFramelessWindow);
#if defined(Q_OS_WIN)
    ui_font_.setPointSize(10);
#else
    ui_font_.setPointSize(12);
#endif
}

void ThemeManager::setThemeColor(ThemeColor theme_color) {
    theme_color_ = theme_color;
    setPalette();
    AppSettings::setEnumValue(kAppSettingTheme, theme_color_);
    play_arrow_ = QIcon(Q_STR(":/xamp/Resource/%1/play_arrow.png").arg(themeColorPath()));
}

QLatin1String ThemeManager::themeColorPath() const {
    if (theme_color_ == ThemeColor::DARK_THEME) {
        return Q_TEXT("Black");
    }
    return Q_TEXT("White");
}

void ThemeManager::setThemeButtonIcon(Ui::XampWindow &ui) {
    switch (themeColor()) {
    case ThemeColor::DARK_THEME:
        ui.themeButton->setIcon(qTheme.darkModeIcon());
        break;
    case ThemeColor::LIGHT_THEME:
        ui.themeButton->setIcon(qTheme.lightModeIcon());
        break;
    }
}

QColor ThemeManager::themeTextColor() const {
    auto color = Qt::black;
    switch (themeColor()) {
    case ThemeColor::DARK_THEME:
        color = Qt::white;
        break;
    case ThemeColor::LIGHT_THEME:
        color = Qt::black;
        break;
    }
    return color;
}

QString ThemeManager::backgroundColor() {
    QString color;

    switch (themeColor()) {
    case ThemeColor::DARK_THEME:
        color = Q_TEXT("#19232D");
        break;
    case ThemeColor::LIGHT_THEME:
        color = Q_TEXT("#FAFAFA");
        break;
    }
    return color;
}

void ThemeManager::setBorderRadius(QFrame* content_widget) {
    auto color = QColor(backgroundColor());
    auto transparent_color = QColor(backgroundColor());

#if XAMP_OS_WIN
    if (AppSettings::getValueAsBool(kAppSettingEnableBlur)) {
        color.setAlpha(220);
        transparent_color.setAlpha(0);
    }
#else
    color.setAlpha(0);
    transparent_color.setAlpha(0);
#endif

    // border-radius: 8px;
	// border: 2px solid % 1;
    content_widget->setStyleSheet(Q_STR(R"(
			QFrame#XampWindow {
				background-color: %1;
            }
			QStackedWidget#currentView {
				border: none;
				background-color: %1;
			}
			QFrame#controlFrame {
				border-radius: 0px;
				background-color: %1;
			}
			QFrame#playingFrame {
				border-radius: 0px;
				background-color: %1;
			}
			QFrame#titleFrame {
				border-radius: 0px;
				background-color: %1;
			}
			QFrame#volumeFrame {
				border-radius: 0px;
				background-color: %1;
			}
			QFrame#sliderFrame {
				border-radius: 0px;
				background-color: %1;
			}
			TabListView#sliderBar {
				border-radius: 0px;	
				background-color: %2;			
			}
            )").arg(colorToString(color)).arg(colorToString(transparent_color)));
}

void ThemeManager::setMenuStyle(QWidget* menu) {
	menu->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    menu->setAttribute(Qt::WA_TranslucentBackground);
    menu->setAttribute(Qt::WA_StyledBackground);
}

DefaultStylePixmapManager::DefaultStylePixmapManager()
    : unknown_cover_(Q_TEXT(":/xamp/Resource/White/unknown_album.png"))
    , default_size_unknown_cover_(Pixmap::resizeImage(unknown_cover_, qTheme.getDefaultCoverSize())) {
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

QIcon ThemeManager::folderIcon() const {
    return QIcon(Q_STR(":/xamp/Resource/%1/folder.png").arg(themeColorPath()));
}

QIcon ThemeManager::speakerIcon() const {
    return QIcon(Q_STR(":/xamp/Resource/%1/speaker.png").arg(themeColorPath()));
}

QIcon ThemeManager::usbIcon() const {
    return QIcon(Q_STR(":/xamp/Resource/%1/usb.png").arg(themeColorPath()));
}

QIcon ThemeManager::playlistIcon() const {
    return makeIcon(Q_STR(":/xamp/Resource/%1/tab_playlists.png"));
}

QIcon ThemeManager::equalizerIcon() const {
    return makeIcon(Q_STR(":/xamp/Resource/%1/equalizer.png"));
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

QIcon ThemeManager::darkModeIcon() const {
    return makeIcon(Q_STR(":/xamp/Resource/%1/dark_mode.png"));	
}

QIcon ThemeManager::lightModeIcon() const {
    return makeIcon(Q_STR(":/xamp/Resource/%1/light_mode.png"));
}

QIcon ThemeManager::seachIcon() const {
    return makeIcon(Q_STR(":/xamp/Resource/%1/search.png"));
}

QIcon ThemeManager::themeIcon() const {
    return makeIcon(Q_STR(":/xamp/Resource/%1/theme.png"));
}

QIcon ThemeManager::desktopIcon() const {
    return makeIcon(Q_STR(":/xamp/Resource/%1/desktop.png"));
}

void ThemeManager::setBitPerfectButton(Ui::XampWindow& ui, bool enable) {
    ui.bitPerfectButton->setText(Q_STR("Bit-Perfect"));

    if (enable) {        
        ui.bitPerfectButton->setStyleSheet(Q_STR(
            R"(
                QToolButton#bitPerfectButton {
					font-family: "FormatFont";
					font-weight: bold;
					color: white;
                    border: none;
                    background-color: rgb(24, 151, 255);
                }
            )"
        ));
    }
    else {        
        ui.bitPerfectButton->setStyleSheet(Q_STR(
            R"(
                QToolButton#bitPerfectButton {
					font-family: "FormatFont";
					font-weight: bold;
					color: white;
                    border: none;
                    background-color: rgba(24, 151, 255, 60);
                }
            )"
        ));
    }
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
    auto color = palette().color(QPalette::WindowText);
    widget->setStyleSheet(backgroundColorToString(color));
}

void ThemeManager::enableBlur(QWidget* widget, bool enable) const {
#if defined(Q_OS_WIN)
    win32::setAccentPolicy(widget, enable);
#else
    osx::setBlurMaterial(widget, enable);
#endif
    AppSettings::setValue(kAppSettingEnableBlur, enable);
}

QIcon ThemeManager::appIcon() const {
    return QIcon(Q_TEXT(":/xamp/xamp.ico"));
}

void ThemeManager::applyTheme() {
    qApp->setFont(defaultFont());

    QString filename;

    if (themeColor() == ThemeColor::LIGHT_THEME) {
        filename = Q_TEXT(":/xamp/Resource/Theme/light/style.qss");
    } else {
        filename = Q_TEXT(":/xamp/Resource/Theme/dark/style.qss");
    }

    QFile f(filename);
    f.open(QFile::ReadOnly | QFile::Text);
    QTextStream ts(&f);
    qApp->setStyleSheet(ts.readAll());
    f.close();
}

void ThemeManager::setBackgroundColor(Ui::XampWindow& ui, QColor color) {
    background_color_ = color;
    AppSettings::setValue(kAppSettingBackgroundColor, color);
}

QIcon ThemeManager::playArrow() const noexcept {
    return play_arrow_;
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
                                         image: url(:/xamp/Resource/%1/close_normal.png);
                                         background-color: transparent;
                                         }

										 QToolButton#closeButton:hover {	
										 image: url(:/xamp/Resource/%1/close_hover.png);									 
                                         }
                                         )").arg(themeColorPath()));

    ui.minWinButton->setStyleSheet(Q_STR(R"(
                                          QToolButton#minWinButton {
                                          border: none;
                                          image: url(:/xamp/Resource/%1/minimize_normal.png);
                                          background-color: transparent;
                                          }
										  QToolButton#minWinButton:hover {	
										  image: url(:/xamp/Resource/%1/minimize_hover.png);									 
                                          }
                                          )").arg(themeColorPath()));

    ui.maxWinButton->setStyleSheet(Q_STR(R"(
                                          QToolButton#maxWinButton {
                                          border: none;
                                          image: url(:/xamp/Resource/%1/maximize_normal.png);
                                          background-color: transparent;
                                          }
										  QToolButton#maxWinButton:hover {	
										  image: url(:/xamp/Resource/%1/maximize_hover.png);									 
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

    ui.themeButton->setStyleSheet(Q_STR(R"(
                                            QToolButton#themeButton {
                                            border: none;
                                            background-color: transparent;
                                            }
                                            QToolButton#themeButton::menu-indicator {
                                            image: none;
                                            }
                                            )"));


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

    ui.eqButton->setStyleSheet(Q_STR(R"(
                                         QToolButton#eqButton {
                                         border: none;
                                         image: url(:/xamp/Resource/%1/equalizer.png);
                                         background-color: transparent;
                                         }
                                         )").arg(themeColorPath()));
}

void ThemeManager::setWidgetStyle(Ui::XampWindow& ui) {
    ui.searchLineEdit->setStyleSheet(Q_TEXT(""));
    ui.sliderBar->setStyleSheet(Q_TEXT("QListView#sliderBar { background-color: transparent; border: none; }"));
    
    ui.searchFrame->setStyleSheet(Q_TEXT("QFrame#searchFrame { background-color: transparent; border: none; }"));

    ui.volumeSlider->setStyleSheet(Q_TEXT(R"(
                                          QSlider#volumeSlider {
                                          border: none;
                                          background-color: transparent;
                                          }
                                          )"));

    ui.startPosLabel->setStyleSheet(Q_TEXT(R"(
                                           QLabel#startPosLabel {
                                           color: gray;
                                           background-color: transparent;
                                           }
                                           )"));

    ui.endPosLabel->setStyleSheet(Q_TEXT(R"(
                                         QLabel#endPosLabel {
                                         color: gray;
                                         background-color: transparent;
                                         }
                                         )"));

    ui.seekSlider->setStyleSheet(Q_TEXT(R"(
                                        QSlider#seekSlider {
                                        border: none;
                                        background-color: transparent;
                                        }
                                        )"));

    ui.sliderBar->setStyleSheet(Q_TEXT(R"(
	QListView#sliderBar::item {
		border: 0px;
		padding-left: 15px;
	}
	QListView#sliderBar::text {
		left: 15px;
	}
	)"));

    ui.titleFrameLabel->setStyleSheet(Q_STR(R"(
    QLabel#titleFrameLabel {
    border: none;
    background: transparent;
	color: gray;
    }
    )"));

    ui.searchLineEdit->setClearButtonEnabled(true);
    if (theme_color_ == ThemeColor::DARK_THEME) {
        ui.titleLabel->setStyleSheet(Q_TEXT(R"(
                                         QLabel#titleLabel {
                                         color: white;
                                         background-color: transparent;
                                         }
                                         )"));

        ui.searchLineEdit->setStyleSheet(Q_STR(R"(
                                            QLineEdit#searchLineEdit {
                                            background-color: %1;
                                            border: gray;
                                            color: white;
                                            border-radius: 10px;
                                            }
                                            )").arg(colorToString(Qt::black)));
    }
    else {
        ui.titleLabel->setStyleSheet(Q_TEXT(R"(
                                         QLabel#titleLabel {
                                         color: black;
                                         background-color: transparent;
                                         }
                                         )"));

        ui.searchLineEdit->setStyleSheet(Q_STR(R"(
                                            QLineEdit#searchLineEdit {
                                            background-color: %1;
                                            border: gray;
                                            color: black;
                                            border-radius: 10px;
                                            }
                                            )").arg(colorToString(Qt::white)));
    }

    ui.artistLabel->setStyleSheet(Q_TEXT(R"(
                                         QLabel#artistLabel {
                                         color: red;
                                         background-color: transparent;
                                         }
                                         )"));

    setThemeIcon(ui);
}
