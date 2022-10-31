#include "ui_xamp.h"

#include <widget/image_utiltis.h>
#include <QGraphicsDropShadowEffect>
#include <QScreen>
#include <QResource>
#include <QPushButton>
#include <QApplication>
#include <QFontDatabase>
#include <QTextStream>
#include <QFileInfo>

#if defined(Q_OS_WIN)
#include <widget/win32/win32.h>
#else
#include <widget/osx/osx.h>
#endif

#include <widget/appsettingnames.h>
#include <widget/widget_shared.h>
#include <widget/str_utilts.h>
#include <widget/appsettings.h>
#include <widget/iconsizestyle.h>
#include "thememanager.h"

bool ThemeManager::useNativeWindow() const {
    return use_native_window_;
}

QString ThemeManager::fontNamePath(const QString& file_name) {
	return
		Q_STR("%1/fonts/%2")
		.arg(QCoreApplication::applicationDirPath())
		.arg(file_name);
}

void ThemeManager::installFileFont(const QString& file_name, QList<QString> &ui_fallback_fonts) {
	const auto font_path = fontNamePath(file_name);
    QFileInfo info(font_path);
    if (!info.exists()) {
        XAMP_LOG_ERROR("Not found font file name: {}", file_name.toStdString());
        return;
    }
    const auto loaded_font_id = QFontDatabase::addApplicationFont(font_path);
    const auto font_families = QFontDatabase::applicationFontFamilies(loaded_font_id);
    ui_fallback_fonts.push_back(font_families[0]);
}

QFont ThemeManager::loadFonts() {
    QList<QString> format_font_families;
    QList<QString> mono_fonts;
    QList<QString> ui_fallback_fonts;

    qFontIcon.addFont(fontNamePath(Q_TEXT("SegoeFluentIcons.ttf")));

    installFileFont(Q_TEXT("RobotoMono-Regular.ttf"), mono_fonts);
    installFileFont(Q_TEXT("Karla-Regular.ttf"), format_font_families);

    installFileFont(Q_TEXT("WorkSans-Regular.ttf"), ui_fallback_fonts);
    installFileFont(Q_TEXT("WorkSans-Bold.ttf"), ui_fallback_fonts);

    installFileFont(Q_TEXT("HarmonyOS_Sans_TC_Regular.ttf"), ui_fallback_fonts);
    installFileFont(Q_TEXT("HarmonyOS_Sans_TC_Bold.ttf"), ui_fallback_fonts);
    installFileFont(Q_TEXT("HarmonyOS_Sans_SC_Regular.ttf"), ui_fallback_fonts);
    installFileFont(Q_TEXT("HarmonyOS_Sans_SC_Bold.ttf"), ui_fallback_fonts);

    QFont::insertSubstitutions(Q_TEXT("FormatFont"), format_font_families);
    QFont::insertSubstitutions(Q_TEXT("MonoFont"), mono_fonts);
    QFont::insertSubstitutions(Q_TEXT("UI"), ui_fallback_fonts);

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
    save_cover_art_size_ = QSize(500, 500);
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
    unknown_cover_ = QPixmap(Q_TEXT(":/xamp/Resource/White/unknown_album.png"));
	default_size_unknown_cover_ = QPixmap(Pixmap::scaledImage(unknown_cover_, cover_size_));
}

void ThemeManager::setThemeColor(ThemeColor theme_color) {
    theme_color_ = theme_color;
    setPalette();
    AppSettings::setEnumValue(kAppSettingTheme, theme_color_);
    switch (themeColor()) {
    case ThemeColor::DARK_THEME:
        qFontIcon.setBaseColor(Qt::white);
        break;
    case ThemeColor::LIGHT_THEME:
        qFontIcon.setBaseColor(Qt::black);
        break;
    }
    play_arrow_ = QIcon(Q_STR(":/xamp/Resource/%1/play_arrow.png").arg(themeColorPath()));
}

QLatin1String ThemeManager::themeColorPath() const {
    if (theme_color_ == ThemeColor::DARK_THEME) {
        return Q_TEXT("Black");
    }
    return Q_TEXT("White");
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

QColor ThemeManager::backgroundColor() const noexcept {
    return background_color_;
}

QString ThemeManager::backgroundColorString() const {
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

void ThemeManager::setMenuStyle(QWidget* menu) {
	menu->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    menu->setAttribute(Qt::WA_TranslucentBackground);
    menu->setAttribute(Qt::WA_StyledBackground);
    menu->setStyle(new IconSizeStyle(12));
}

QIcon ThemeManager::volumeUp() const {
    return iconFromFont(IconCode::ICON_VolumeUp);
}

QIcon ThemeManager::volumeOff() const {
    return iconFromFont(IconCode::ICON_VolumeOff);
}

QIcon ThemeManager::iconFromFont(const QChar& code) const {
    return qFontIcon.icon(code);
}

QIcon ThemeManager::makeIcon(const QString &path) const {
    QIcon icon;
    icon.addPixmap(path.arg(themeColorPath()), QIcon::Normal);
    icon.addPixmap(path.arg(themeColorPath()), QIcon::Selected);
    return icon;
}

QIcon ThemeManager::folderIcon() const {
    return iconFromFont(IconCode::ICON_Folder);
}

QIcon ThemeManager::speakerIcon() const {
    return iconFromFont(IconCode::ICON_Speaker);
}

QIcon ThemeManager::playlistIcon() const {
    return iconFromFont(IconCode::ICON_Playlist);
}

QIcon ThemeManager::equalizerIcon() const {
    return iconFromFont(IconCode::ICON_Equalizer);
}

QIcon ThemeManager::podcastIcon() const {
    return iconFromFont(IconCode::ICON_Podcast);
}

QIcon ThemeManager::albumsIcon() const {
    return iconFromFont(IconCode::ICON_Album);
}

QIcon ThemeManager::cdIcon() const {
    return iconFromFont(IconCode::ICON_CD);
}

QIcon ThemeManager::artistsIcon() const {
    return iconFromFont(IconCode::ICON_Artist);
}

QIcon ThemeManager::subtitleIcon() const {
    return iconFromFont(IconCode::ICON_Subtitle);
}

QIcon ThemeManager::preferenceIcon() const  {
    return iconFromFont(IconCode::ICON_Preference);
}

QIcon ThemeManager::aboutIcon() const {
    return iconFromFont(IconCode::ICON_About);
}

QIcon ThemeManager::darkModeIcon() const {
    return iconFromFont(IconCode::ICON_DarkMode);
}

QIcon ThemeManager::lightModeIcon() const {
    return iconFromFont(IconCode::ICON_LightMode);
}

QIcon ThemeManager::seachIcon() const {
    return iconFromFont(IconCode::ICON_Search);
}

QIcon ThemeManager::themeIcon() const {
    return iconFromFont(IconCode::ICON_Theme);
}

QIcon ThemeManager::desktopIcon() const {
    return iconFromFont(IconCode::ICON_Desktop);
}

QIcon ThemeManager::appIcon() const {
    return QIcon(Q_TEXT(":/xamp/xamp.ico"));
}

QIcon ThemeManager::playCircleIcon() const {
    return QIcon(Q_TEXT(":/xamp/Resource/Black/play_circle.png"));
    //Color color(Qt::white);
    //return Q_ICON_FONT_CODE_COLOR(0xe1c4, &color);
}

QIcon ThemeManager::moreIcon() const {
    return iconFromFont(IconCode::ICON_More);
}

void ThemeManager::setShufflePlayorder(Ui::XampWindow& ui) const {
    ui.repeatButton->setIcon(iconFromFont(IconCode::ICON_ShufflePlayOrder));
}

void ThemeManager::setRepeatOnePlayOrder(Ui::XampWindow& ui) const {
    ui.repeatButton->setIcon(iconFromFont(IconCode::ICON_RepeatOnePlayOrder));
}

void ThemeManager::setRepeatOncePlayOrder(Ui::XampWindow& ui) const {
    ui.repeatButton->setIcon(iconFromFont(IconCode::ICON_RepeatOncePlayOrder));
}

QIcon ThemeManager::minimizeWindowIcon() const {
    return iconFromFont(IconCode::ICON_MinimizeWindow);
}

QIcon ThemeManager::maximumWindowIcon() const {
    return iconFromFont(IconCode::ICON_MaximumWindow);
}

QIcon ThemeManager::closeWindowIcon() const {
    return iconFromFont(IconCode::ICON_CloseWindow);
}

QIcon ThemeManager::restoreWindowIcon() const {
    return iconFromFont(IconCode::ICON_RestoreWindow);
}

QIcon ThemeManager::sliderBarIcon() const {
    return iconFromFont(IconCode::ICON_SliderBar);
}

QPixmap ThemeManager::githubIcon() const {
	if (themeColor() == ThemeColor::DARK_THEME) {
        return QPixmap(Q_TEXT(":/xamp/Resource/Black/GitHub-Mark.png"));
	} else {
        return QPixmap(Q_TEXT(":/xamp/Resource/White/GitHub-Mark.png"));
	}
}

const QPixmap& ThemeManager::unknownCover() const noexcept {
    return unknown_cover_;
}

const QPixmap& ThemeManager::defaultSizeUnknownCover() const noexcept {
    return default_size_unknown_cover_;
}

void ThemeManager::updateMaximumIcon(Ui::XampWindow& ui, bool is_maximum) const {
    if (is_maximum) {
        ui.maxWinButton->setIcon(restoreWindowIcon());
    } else {
        ui.maxWinButton->setIcon(maximumWindowIcon());
    }
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
        ui.playButton->setIcon(iconFromFont(IconCode::ICON_Pause));
    }
    else {
        ui.playButton->setIcon(iconFromFont(IconCode::ICON_Play));
    }
}

const QSize& ThemeManager::defaultCoverSize() const noexcept {
    return cover_size_;
}

QSize ThemeManager::cacheCoverSize() const noexcept {
    return defaultCoverSize() * 2;
}

QSize ThemeManager::albumCoverSize() const noexcept {
    return album_cover_size_;
}

QSize ThemeManager::saveCoverArtSize() const noexcept {
    return save_cover_art_size_;
}

void ThemeManager::setBackgroundColor(QWidget* widget) {
    auto color = palette().color(QPalette::WindowText);
    widget->setStyleSheet(backgroundColorToString(color));
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

QIcon ThemeManager::playArrow() const {
    return play_arrow_;
}

QColor ThemeManager::titleBarColor() const {
    return QColor(backgroundColor());
}

QColor ThemeManager::coverShadownColor() const {
    switch (themeColor()) {
    case ThemeColor::DARK_THEME:
        return Qt::black;
    case ThemeColor::LIGHT_THEME:
    default:
        return QColor(Q_TEXT("#DCDCDC"));
    }
}

QSize ThemeManager::tabIconSize() const {
#ifdef XAMP_OS_MAC
    return QSize(20, 20);
#else
    return QSize(18, 18);
#endif
}

void ThemeManager::updateTitlebarState(QFrame *title_bar, bool is_focus) {
    /*if (!is_focus) {
        QColor title_bar_color = titleBarColor();
        title_bar_color = title_bar_color.light();

        title_bar->setStyleSheet(Q_STR(R"(
			QFrame#titleFrame {
				background-color: %1;
				border: none;
				border-radius: 0px;
            }			
            )").arg(colorToString(title_bar_color)));
    }
    else {
        title_bar->setStyleSheet(Q_STR(R"(
			QFrame#titleFrame {
				border: none;
				border-radius: 0px;
            }			
            )"));
    }*/
}

QColor ThemeManager::hoverColor() const {
    switch (themeColor()) {
    case ThemeColor::DARK_THEME:
        return QColor(Q_TEXT("#455364"));
    case ThemeColor::LIGHT_THEME:
    default:
        return QColor(Q_TEXT("#C9CDD0"));
    }
}

void ThemeManager::setStandardButtonStyle(QToolButton* close_button, QToolButton* min_win_button, QToolButton* max_win_button) const {
    const QColor hover_color = hoverColor();
    auto font_size = 10;

    close_button->setStyleSheet(Q_STR(R"(
                                         QToolButton#closeButton {
                                         border: none;
                                         background-color: transparent;
										 border-radius: 0px;
                                         }

										 QToolButton#closeButton:hover {
										 background-color: %1;
										 border-radius: 0px;
                                         }
                                         )").arg(colorToString(hover_color)));
    close_button->setIconSize(QSize(font_size, font_size));
    close_button->setIcon(closeWindowIcon());

    min_win_button->setStyleSheet(Q_STR(R"(
                                          QToolButton#minWinButton {
                                          border: none;
                                          background-color: transparent;
										  border-radius: 0px;
                                          }
										  QToolButton#minWinButton:hover {		
										  background-color: %1;
										  border-radius: 0px;				 
                                          }
                                          )").arg(colorToString(hover_color)));
    min_win_button->setIconSize(QSize(font_size, font_size));
    min_win_button->setIcon(minimizeWindowIcon());

    max_win_button->setStyleSheet(Q_STR(R"(
                                          QToolButton#maxWinButton {
                                          border: none;
                                          background-color: transparent;
										  border-radius: 0px;
                                          }
										  QToolButton#maxWinButton:hover {		
										  background-color: %1;
										  border-radius: 0px;								 
                                          }
                                          )").arg(colorToString(hover_color)));
    max_win_button->setIconSize(QSize(font_size, font_size));
    max_win_button->setIcon(maximumWindowIcon());
}

void ThemeManager::setThemeIcon(Ui::XampWindow& ui) const {
    setStandardButtonStyle(ui.closeButton, ui.minWinButton, ui.maxWinButton);

    const QColor hover_color = hoverColor();

    if (useNativeWindow()) {
        ui.logoButton->hide();
    } else {
        ui.logoButton->setStyleSheet(Q_STR(R"(
                                         QToolButton#logoButton {
                                         border: none;
                                         image: url(":/xamp/xamp.ico");
                                         background-color: transparent;
                                         }
										)"));
    }

    ui.sliderBarButton->setStyleSheet(Q_STR(R"(
                                            QToolButton#sliderBarButton {
                                            border: none;
                                            background-color: transparent;
											border-radius: 0px;
                                            }
											QToolButton#sliderBarButton:hover {												
											background-color: %1;
											border-radius: 0px;								 
											}
                                            QToolButton#sliderBarButton::menu-indicator {
                                            image: none;
                                            }
                                            )").arg(colorToString(hover_color)));
    ui.sliderBarButton->setIcon(sliderBarIcon());

    ui.stopButton->setStyleSheet(Q_STR(R"(
                                         QToolButton#stopButton {
                                         border: none;
                                         background-color: transparent;
                                         }
                                         )"));
    ui.stopButton->setIcon(iconFromFont(ICON_Stop));

    ui.nextButton->setStyleSheet(Q_TEXT(R"(
                                        QToolButton#nextButton {
                                        border: none;
                                        background-color: transparent;
                                        }
                                        )"));
    ui.nextButton->setIcon(iconFromFont(IconCode::ICON_PlayNext));

    ui.prevButton->setStyleSheet(Q_TEXT(R"(
                                        QToolButton#prevButton {
                                        border: none;
                                        background-color: transparent;
                                        }
                                        )"));
    ui.prevButton->setIcon(iconFromFont(IconCode::ICON_PlayPrev));

    ui.selectDeviceButton->setStyleSheet(Q_TEXT(R"(
                                                QToolButton#selectDeviceButton {                                                
                                                border: none;
                                                background-color: transparent;                                                
                                                }
                                                QToolButton#selectDeviceButton::menu-indicator { image: none; }
                                                )"));
    ui.selectDeviceButton->setIcon(speakerIcon());

    ui.mutedButton->setStyleSheet(Q_STR(R"(
                                         QToolButton#mutedButton {
                                         image: url(:/xamp/Resource/%1/volume_up.png);
                                         border: none;
                                         background-color: transparent;
                                         }
                                         )").arg(themeColorPath()));

    ui.eqButton->setStyleSheet(Q_TEXT(R"(
                                         QToolButton#eqButton {
                                         border: none;
                                         background-color: transparent;
                                         }
                                         )"));
    ui.eqButton->setIcon(equalizerIcon());
}

void ThemeManager::setTextSeparator(QFrame *frame) {
    switch (themeColor()) {
    case ThemeColor::DARK_THEME:
        frame->setStyleSheet(Q_TEXT("background-color: #37414F;"));
        break;
    case ThemeColor::LIGHT_THEME:
        frame->setStyleSheet(Q_TEXT("background-color: #CED1D4;"));
        break;
    }
}

void ThemeManager::setWidgetStyle(Ui::XampWindow& ui) {
    QString slider_bar_left_color;
    switch (themeColor()) {
    case ThemeColor::DARK_THEME:
        slider_bar_left_color = Q_TEXT("42, 130, 218");
        break;
    case ThemeColor::LIGHT_THEME:
        slider_bar_left_color = Q_TEXT("42, 130, 218");
        break;
    }

    ui.playButton->setIconSize(QSize(32, 32));
    ui.selectDeviceButton->setIconSize(QSize(32, 32));
    ui.mutedButton->setIconSize(QSize(32, 32));

    ui.playButton->setStyleSheet(Q_TEXT(R"(
                                            QToolButton#playButton {
                                            border: none;
                                            background-color: transparent;
                                            }
                                            )"));

    ui.searchLineEdit->setStyleSheet(Q_TEXT(""));

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

    ui.sliderBar->setStyleSheet(Q_STR(R"(
	QListView#sliderBar {
		border: none; 
	}
	QListView#sliderBar::item {
		border: 0px;
		padding-left: 6px;
	}
	QListView#sliderBar::item:hover {
		border-radius: 2px;
	}
	QListView#sliderBar::item:selected {
		padding-left: 4px;
		border-left-width: 2px;
		border-left-style: solid;
		border-left-color: rgb(%1);
	}	
	)").arg(slider_bar_left_color));

    ui.titleFrameLabel->setStyleSheet(Q_STR(R"(
    QLabel#titleFrameLabel {
    border: none;
    background: transparent;
	color: gray;
    }
    )"));

    ui.tableLabel->setStyleSheet(Q_STR(R"(
    QLabel#tableLabel {
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

        ui.currentView->setStyleSheet(Q_STR(R"(
			QStackedWidget#currentView {
				background-color: #121920;
				border: 1px solid black;
				border-top-left-radius: 8px;
            }			
            )"));

        ui.bottomFrame->setStyleSheet(
            Q_TEXT(R"(
            QFrame#bottomFrame{
                border-top: 1px solid black;
                border-radius: 0px;
				border-bottom: none;
				border-left: none;
				border-right: none;
            }
            )"));
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

        ui.currentView->setStyleSheet(Q_TEXT(R"(
			QStackedWidget#currentView {
				background-color: #f9f9f9;
				border: 1px solid #eaeaea;
				border-top-left-radius: 8px;
            }			
            )"));

        ui.bottomFrame->setStyleSheet(
            Q_TEXT(R"(
            QFrame#bottomFrame {
                border-top: 1px solid #eaeaea;
                border-radius: 0px;
				border-bottom: none;
				border-left: none;
				border-right: none;
            }
            )"));
    }

    ui.artistLabel->setStyleSheet(Q_TEXT(R"(
                                         QLabel#artistLabel {
                                         color: rgb(250, 88, 106);
                                         background-color: transparent;
                                         }
                                         )"));

    ui.repeatButton->setStyleSheet(Q_TEXT(R"(
    QToolButton#repeatButton {
    border: none;
    background: transparent;
    }
    )"
    ));
   
    setThemeIcon(ui);
    ui.sliderBarButton->setIconSize(tabIconSize());
    ui.sliderFrame->setStyleSheet(Q_TEXT("background: transparent; border: none;"));    
    ui.currentViewFrame->setStyleSheet(Q_TEXT("border: none;"));
}
