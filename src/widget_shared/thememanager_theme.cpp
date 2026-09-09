//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#include <thememanager.h>

#include <widget/appsettingnames.h>
#include <widget/appsettings.h>
#include <widget/util/image_util.h>
#include <widget/util/str_util.h>
#include <base/logger.h>

#include <QApplication>
#include <QColor>
#include <QFile>
#include <QLatin1String>
#include <QPalette>
#include <QSize>
#include <QTextStream>

void ThemeManager::setPalette() {
    palette_ = QPalette();
    const bool dark = isDarkTheme();
    background_color_ = dark ? QColor("#141719"_str) : QColor("#E9EEEB"_str);
    palette_.setColor(QPalette::Window, background_color_);
    palette_.setColor(QPalette::WindowText, dark ? QColor("#EEF1EF"_str) : QColor("#202623"_str));
    palette_.setColor(QPalette::Text, palette_.color(QPalette::WindowText));
    palette_.setColor(QPalette::ButtonText, palette_.color(QPalette::WindowText));
    palette_.setColor(QPalette::Base, dark ? QColor("#191D20"_str) : QColor("#FFFFFF"_str));
    palette_.setColor(QPalette::Button, dark ? QColor("#252D2F"_str) : QColor("#EEF2EF"_str));
    palette_.setColor(QPalette::Highlight, highlightColor());
    palette_.setColor(QPalette::HighlightedText, dark ? QColor("#C6F0E1"_str) : QColor("#202623"_str));
}

void ThemeManager::setThemeColor(ThemeColor theme_color, bool notify) {
    XAMP_LOG_DEBUG("setThemeColor");

    theme_color_ = theme_color;

    setPalette();

    qAppSettings.setEnumValue(kAppSettingTheme, theme_color_);

    font_icon_opts_.clear();

    switch (theme_color) {
    case ThemeColor::DARK_THEME:
        font_icon_opts_.insert(FontIconOption::kColorAttr, QVariant(QColor(240, 241, 243)));
        font_icon_opts_.insert(FontIconOption::kSelectedColorAttr, QVariant(QColor(240, 241, 243)));
        unknown_cover_ = QPixmap(":/xamp/Resource/Black/unknown_album.png"_str);
        break;
    case ThemeColor::LIGHT_THEME:
        font_icon_opts_.insert(FontIconOption::kColorAttr, QVariant(QColor(97, 97, 101)));
        font_icon_opts_.insert(FontIconOption::kSelectedColorAttr, QVariant(QColor(97, 97, 101)));
        unknown_cover_ = QPixmap(":/xamp/Resource/White/unknown_album.png"_str);
        break;
    }
    default_size_unknown_cover_ = image_util::resizeImage(unknown_cover_, album_cover_size_, true);
    if (notify) {
        setThemeQssFile();
        emit themeChangedFinished(theme_color);
    }
}

QLatin1String ThemeManager::themeColorPath() const {
    return themeColorPath(theme_color_);
}

QLatin1String ThemeManager::themeColorPath(ThemeColor theme_color) const {
    if (theme_color == ThemeColor::DARK_THEME) {
		return "Black"_str;
	}
	return "White"_str;
}

QColor ThemeManager::indicatorColor() const {
    return isDarkTheme() ? QColor("#94D8C3"_str) : textColor();
    //return QColor(232, 214, 90);
}

QColor ThemeManager::textColor() const {
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

QColor ThemeManager::backgroundColor() const {
    return background_color_;
}

QString ThemeManager::backgroundColorString() const {
    QString color;

    switch (themeColor()) {
    case ThemeColor::DARK_THEME:
        color = "#141719"_str;
        break;
    case ThemeColor::LIGHT_THEME:
        color = "#E9EEEB"_str;
        break;
    }
    return color;
}

const QSize& ThemeManager::defaultCoverSize() const {
    return cover_size_;
}

QSize ThemeManager::cacheCoverSize() const {
    return cache_cover_size_;
}

QSize ThemeManager::albumCoverSize() const {
    return album_cover_size_;
}

void ThemeManager::setThemeQssFile() {
    qApp->setFont(defaultFont());
    qApp->setPalette(palette_);

    QString filename;

    if (themeColor() == ThemeColor::LIGHT_THEME) {
        filename = ":/xamp/Resource/Theme/light/lightstyle.qss"_str;
    } else {
        filename = ":/xamp/Resource/Theme/dark/darkstyle.qss"_str;
    }

    QFile f(filename);
    f.open(QFile::ReadOnly | QFile::Text);
    QTextStream ts(&f);
    ts.setEncoding(QStringConverter::Utf8);
    QString stylesheet = ts.readAll();
    {
        QFile modern(isDarkTheme() ? ":/xamp/Resource/Theme/dark/modern.qss"_str
            : ":/xamp/Resource/Theme/light/modern.qss"_str);
        if (modern.open(QFile::ReadOnly | QFile::Text)) {
            stylesheet += QString::fromUtf8(modern.readAll());
        }
    }
    qApp->setStyleSheet(stylesheet);

    f.close();
}

void ThemeManager::setBackgroundColor(QColor color) {
    background_color_ = color;
    qAppSettings.setValue(kAppSettingBackgroundColor, color);
}

QColor ThemeManager::titleBarColor() const {
    return QColor(backgroundColor());
}

QColor ThemeManager::coverShadowColor() const {
    switch (themeColor()) {
    case ThemeColor::DARK_THEME:
    case ThemeColor::LIGHT_THEME:
    default:
        return {"#DCDCDC"_str};
    }
}

QString ThemeManager::linearGradientStyle() const {
    switch (themeColor()) {
    default:
    case ThemeColor::DARK_THEME:
        return "#2e2f31"_str;
    case ThemeColor::LIGHT_THEME:
        return "#ffffff"_str;
    }
}

QSize ThemeManager::tabIconSize() const {
#ifdef XAMP_OS_MAC
    return QSize(20, 20);
#else
    return QSize(18, 18);
#endif
}

QColor ThemeManager::hoverColor() const {
    switch (themeColor()) {
    case ThemeColor::DARK_THEME:
        return {"#252D2F"_str };
    case ThemeColor::LIGHT_THEME:
    default:
        return {"#DCE6DF"_str };
    }
}

QColor ThemeManager::highlightColor() const {
    switch (themeColor()) {
    case ThemeColor::LIGHT_THEME:
        return {"#C9DDD2"_str};
    case ThemeColor::DARK_THEME:
    default:
        return {"#29463E"_str };
    }
}

int32_t ThemeManager::titleBarIconHeight() {
    return 8;
}

