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

bool ThemeManager::enableBlur() const {
    return AppSettings::getValueAsBool(kAppSettingEnableBlur);
}

QString ThemeManager::getFontNamePath(const QString& file_name) {
	return
		Q_STR("%1/fonts/%2")
		.arg(QCoreApplication::applicationDirPath())
		.arg(file_name);
}

void ThemeManager::installFileFont(const QString& file_name, QList<QString> &ui_fallback_fonts) {
	const auto font_path = getFontNamePath(file_name);
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

    qFontIcon.addFont(getFontNamePath(Q_TEXT("MaterialIcons-Regular.ttf")));
    mdl2_font_icon_.addFont(getFontNamePath(Q_TEXT("SegoeFluentIcons.ttf")));

    //installFileFont(Q_TEXT("Electrolize.ttf"), digital_font_families);
    installFileFont(Q_TEXT("RobotoMono-Regular.ttf"), mono_fonts);
    installFileFont(Q_TEXT("Karla-Regular.ttf"), format_font_families);
    //installFileFont(Q_TEXT("Karla-Regular.ttf"), mono_fonts);

    installFileFont(Q_TEXT("Karla_Regular.ttf"), ui_fallback_fonts);
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
        mdl2_font_icon_.setBaseColor(Qt::white);
        break;
    case ThemeColor::LIGHT_THEME:
        qFontIcon.setBaseColor(Qt::black);
        mdl2_font_icon_.setBaseColor(Qt::black);
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

QString ThemeManager::backgroundColor() const {
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
    menu->setStyle(new IconSizeStyle(14));
}

QIcon ThemeManager::volumeUp() const {
    return Q_FONT_ICON_CODE(0xe050);
}

QIcon ThemeManager::volumeOff() const {
    return Q_FONT_ICON_CODE(0xe04f);
}

QIcon ThemeManager::makeIcon(const QString &path) const {
    QIcon icon;
    icon.addPixmap(path.arg(themeColorPath()), QIcon::Normal);
    icon.addPixmap(path.arg(themeColorPath()), QIcon::Selected);
    return icon;
}

QIcon ThemeManager::folderIcon() const {
    return Q_FONT_ICON_CODE(0xe2c7);
}

QIcon ThemeManager::speakerIcon() const {
    return Q_FONT_ICON_CODE(0xe32d);
}

QIcon ThemeManager::usbIcon() const {
    return Q_FONT_ICON_CODE(0xe1e0);
}

QIcon ThemeManager::playlistIcon() const {
    return Q_FONT_ICON_CODE(0xe03d);
}

QIcon ThemeManager::equalizerIcon() const {
    return Q_FONT_ICON_CODE(0xe01d);
}

QIcon ThemeManager::podcastIcon() const {
    return Q_FONT_ICON_CODE(0xf048);
}

QIcon ThemeManager::albumsIcon() const {
    return Q_FONT_ICON_CODE(0xe030);
}

QIcon ThemeManager::cdIcon() const {
    return Q_FONT_ICON_CODE(0xe019);
}

QIcon ThemeManager::artistsIcon() const {
    return Q_FONT_ICON_CODE(0xe7ef);
}

QIcon ThemeManager::subtitleIcon() const {
    return Q_FONT_ICON_CODE(0xec0b);
}

QIcon ThemeManager::preferenceIcon() const  {
    return Q_FONT_ICON_CODE(0xe8b8);
}

QIcon ThemeManager::aboutIcon() const {
    return Q_FONT_ICON_CODE(0xe887);
}

QIcon ThemeManager::darkModeIcon() const {
    return Q_FONT_ICON_CODE(0xe51c);
}

QIcon ThemeManager::lightModeIcon() const {
    return Q_FONT_ICON_CODE(0xe518);
}

QIcon ThemeManager::seachIcon() const {
    return Q_FONT_ICON_CODE(0xe8b6);
}

QIcon ThemeManager::themeIcon() const {
    return Q_FONT_ICON_CODE(0xe40a);
}

QIcon ThemeManager::desktopIcon() const {
    return Q_FONT_ICON_CODE(0xe30a);
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
    return Q_FONT_ICON_CODE(0xe5d4);
}

void ThemeManager::setShufflePlayorder(Ui::XampWindow& ui) const {
    ui.repeatButton->setIcon(Q_FONT_ICON_CODE(0xe043));
}

void ThemeManager::setRepeatOnePlayOrder(Ui::XampWindow& ui) const {
    ui.repeatButton->setIcon(Q_FONT_ICON_CODE(0xe040));
}

void ThemeManager::setRepeatOncePlayOrder(Ui::XampWindow& ui) const {
    ui.repeatButton->setIcon(Q_FONT_ICON_CODE(0xe041));
}

QIcon ThemeManager::minimizeWindowIcon() const {
    return mdl2_font_icon_.icon(0xE921);
}

QIcon ThemeManager::maximumWindowIcon() const {
    return mdl2_font_icon_.icon(0xE922);
}

QIcon ThemeManager::closeWindowIcon() const {
    return mdl2_font_icon_.icon(0xE8BB);
}

QIcon ThemeManager::restoreWindowIcon() const {
    return mdl2_font_icon_.icon(0xE923);
}

QIcon ThemeManager::sliderBarIcon() const {
    return Q_FONT_ICON_CODE(0xe5d2);
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
        ui.playButton->setIcon(Q_FONT_ICON_CODE(0xe1a2));
    }
    else {
        ui.playButton->setIcon(Q_FONT_ICON_CODE(0xe1c4));
    }
}

const QSize& ThemeManager::getDefaultCoverSize() const noexcept {
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

QSize ThemeManager::getSaveCoverArtSize() const noexcept {
    return save_cover_art_size_;
}

void ThemeManager::setBackgroundColor(QWidget* widget) {
    auto color = palette().color(QPalette::WindowText);
    widget->setStyleSheet(backgroundColorToString(color));
}

void ThemeManager::enableBlur(QWidget* widget, bool enable) const {
#if defined(Q_OS_WIN)
    win32::setAccentPolicy(widget->winId(), enable);
#else
    osx::setBlurMaterial(widget, enable);
#endif
    AppSettings::setValue(kAppSettingEnableBlur, enable);
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
#ifdef Q_OS_WIN
    return QSize(18, 18);
#else
    return QSize(24, 24);
#endif
}

void ThemeManager::updateTitlebarState(QFrame *title_bar, bool is_focus) {
    if (!is_focus) {
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
    }
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
    auto font_size = 13;

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

    ui.settingsButton->setStyleSheet(Q_STR(R"(
                                            QToolButton#settingsButton {
                                            border: none;
                                            background-color: transparent;
											border-radius: 0px;
                                            }
											QToolButton#settingsButton:hover {												
											background-color: %1;
											border-radius: 0px;								 
											}
                                            QToolButton#settingsButton::menu-indicator {
                                            image: none;
                                            }
                                            )").arg(colorToString(hover_color)));
    ui.settingsButton->setIcon(moreIcon());

    ui.themeButton->setStyleSheet(Q_STR(R"(
                                            QToolButton#themeButton {
                                            border: none;
                                            background-color: transparent;
                                            }
											QToolButton#themeButton:hover {												
											background-color: %1;
											border-radius: 0px;								 
											}
                                            QToolButton#themeButton::menu-indicator {
                                            image: none;
                                            }
                                            )").arg(colorToString(hover_color)));


    ui.stopButton->setStyleSheet(Q_STR(R"(
                                         QToolButton#stopButton {
                                         border: none;
                                         background-color: transparent;
                                         }
                                         )"));
    ui.stopButton->setIcon(Q_FONT_ICON_CODE(0xe047));

    ui.nextButton->setStyleSheet(Q_TEXT(R"(
                                        QToolButton#nextButton {
                                        border: none;
                                        background-color: transparent;
                                        }
                                        )"));
    ui.nextButton->setIcon(Q_FONT_ICON_CODE(0xe044));

    ui.prevButton->setStyleSheet(Q_TEXT(R"(
                                        QToolButton#prevButton {
                                        border: none;
                                        background-color: transparent;
                                        }
                                        )"));
    ui.prevButton->setIcon(Q_FONT_ICON_CODE(0xe045));

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

void ThemeManager::setWidgetStyle(Ui::XampWindow& ui) {
    ui.playButton->setIconSize(QSize(64, 64));
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

	QString slider_bar_left_color;
	switch (themeColor()) {
	case ThemeColor::DARK_THEME:
        slider_bar_left_color = Q_TEXT("42, 130, 218");
        ui.controlFrame->setStyleSheet(
            Q_TEXT(R"(
            QFrame#controlFrame{
                border-top: 1px solid rgb(22, 22, 22);
				border-radius: 0px;
            }
            )"));
		break;
	case ThemeColor::LIGHT_THEME:
        slider_bar_left_color = Q_TEXT("42, 130, 218");
        ui.controlFrame->setStyleSheet(
            Q_TEXT(R"(
            QFrame#controlFrame{
                border-top: 1px solid lightgray;
				border-radius: 0px;
            }
            )"));
        break;
	}

    ui.sliderBar->setStyleSheet(Q_STR(R"(
	QListView#sliderBar {
		border: none; 
	}
	QListView#sliderBar::item {
		border: 0px;
		padding-left: 6px;
	}
	QListView#sliderBar::item:hover {
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
}
