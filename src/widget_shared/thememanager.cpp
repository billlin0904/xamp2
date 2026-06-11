#include <thememanager.h>

#include <widget/appsettingnames.h>
#include <widget/appsettings.h>

void ThemeManager::load() {
	qTheme; // In widget-shared.dll initialize.
    const auto theme = qAppSettings.valueAsEnum<ThemeColor>(kAppSettingTheme);
    qTheme.setThemeQssFile();
    qTheme.setThemeColor(theme);
}

ThemeManager::ThemeManager() {
    cover_size_ = QSize(185, 185);
    cache_cover_size_ = QSize(544, 544);
    album_cover_size_ = QSize(206, 206);
    save_cover_art_size_ = QSize(350, 350);
    ui_font_ = loadFonts();
#ifdef Q_OS_WIN
    font_ratio_ = 1.0;
#else
    font_ratio_ = 1.4;
#endif
    ui_font_.setPointSize(defaultFontSize());
    setGoogleMaterialFontIcons();
    auto theme_color = qAppSettings.valueAsEnum<ThemeColor>(kAppSettingTheme);
    setThemeColor(theme_color, false);
}

