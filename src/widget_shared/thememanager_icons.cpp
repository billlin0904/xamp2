//=====================================================================================================================
// Copyright (c) 2018-2026 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#include <thememanager.h>

#include <widget/appsettingnames.h>
#include <widget/appsettings.h>
#include <widget/util/str_util.h>
#include <base/logger.h>

#include <QAbstractButton>
#include <QColor>
#include <QCoreApplication>
#include <QIcon>
#include <QPixmap>
#include <QSize>
#include <QToolButton>
#include <QVariantMap>

#include <optional>

QString ThemeManager::countryFlagFilePath(const QString& country_iso_code) {
    return
        qFormat("%1/flags/%2.png")
        .arg(QCoreApplication::applicationDirPath())
        .arg(country_iso_code);
}

QIcon ThemeManager::fontRawIcon(const Glyphs code) {
    return qFontIcon.getIcon(static_cast<int32_t>(code), font_icon_opts_);
}

QIcon ThemeManager::fontRawIconOption(const Glyphs code, const QVariantMap& options) {
    return qFontIcon.getIcon(static_cast<int32_t>(code), options);
}

QIcon ThemeManager::fontIcon(const Glyphs code, std::optional<ThemeColor> theme_color) {
    auto color = theme_color ? *theme_color : themeColor();

    switch (code) {
    case Glyphs::ICON_HEART_PRESS:
    case Glyphs::ICON_HEART:
    {
        auto temp = font_icon_opts_;
        if (code == Glyphs::ICON_HEART) {
            temp.insert(FontIconOption::kColorAttr, QVariant(QColor(128, 128, 128)));
        }
        else {
            temp.insert(FontIconOption::kColorAttr, QVariant(QColor(255, 0, 0)));
        }
        return qFontIcon.getIcon(static_cast<int32_t>(code), temp);
    }
    case Glyphs::ICON_MINIMIZE_WINDOW:
	    {
        auto temp = font_icon_opts_;
        temp.insert(FontIconOption::kColorAttr, QVariant(color != ThemeColor::DARK_THEME ? QColor(Qt::black) : QColor(Qt::gray)));
        temp.insert(FontIconOption::kScaleFactorAttr,1.2);
        return qFontIcon.getIcon(static_cast<int32_t>(code), temp);
	    }
    case Glyphs::ICON_MAXIMUM_WINDOW:
        return QIcon(qFormat(":/xamp/Resource/%1/maximize-active.ico").arg(themeColorPath(color)));
    case Glyphs::ICON_CLOSE_WINDOW:
        return QIcon(qFormat(":/xamp/Resource/%1/close-active.ico").arg(themeColorPath(color)));
    case Glyphs::ICON_RESTORE_WINDOW:
        return QIcon(qFormat(":/xamp/Resource/%1/restore-active.ico").arg(themeColorPath(color)));
    case Glyphs::ICON_MESSAGE_BOX_WARNING:
	    {
			auto temp = font_icon_opts_;
            temp.insert(FontIconOption::kColorAttr, QVariant(QColor(255, 164, 6)));
            temp.insert(FontIconOption::kScaleFactorAttr, 0.8);
            return qFontIcon.getIcon(static_cast<int32_t>(code), temp);
	    }
    case Glyphs::ICON_MESSAGE_BOX_ERROR:
		{
			auto temp = font_icon_opts_;
			temp.insert(FontIconOption::kColorAttr, QVariant(QColor(189, 29, 29)));
			return qFontIcon.getIcon(static_cast<int32_t>(code), temp);
		}
    case Glyphs::ICON_MESSAGE_BOX_INFORMATION:
		{
			auto temp = font_icon_opts_;
			temp.insert(FontIconOption::kColorAttr, QVariant(QColor(43, 128, 234)));
		    return qFontIcon.getIcon(static_cast<int32_t>(code), temp);
		}
    case Glyphs::ICON_MESSAGE_BOX_QUESTION:
		{
			auto temp = font_icon_opts_;
			temp.insert(FontIconOption::kColorAttr, QVariant(QColor(53, 193, 31)));
			return qFontIcon.getIcon(static_cast<int32_t>(code), temp);
		}
    case Glyphs::ICON_MESSAGE_BOX_SUCCESS:
		{
			auto temp = font_icon_opts_;
			temp.insert(FontIconOption::kColorAttr, QVariant(QColor(0, 249, 0)));
			return qFontIcon.getIcon(static_cast<int32_t>(code), temp);
		}
    case Glyphs::ICON_CIRCLE_CHECK:
		{
			auto temp = font_icon_opts_;
			temp.insert(FontIconOption::kColorAttr, QVariant(highlightColor()));
			return qFontIcon.getIcon(static_cast<int32_t>(code), temp);
		}
    }

    if (font_icon_opts_.isEmpty()) {
        XAMP_LOG_DEBUG("font_icon_opts_ is empty.");
    }

    return qFontIcon.getIcon(static_cast<int32_t>(code), font_icon_opts_);
}

QIcon ThemeManager::applicationIcon() const {
#ifdef Q_OS_WIN
    return QIcon(":/xamp/xamp.ico"_str);
#else
    return QIcon(":/xamp/xamp2.png"_str);
#endif
}

QIcon ThemeManager::playCircleIcon() const {
    return QIcon(":/xamp/Resource/Black/play_circle.png"_str);
}

QIcon ThemeManager::playlistPauseIcon(QSize icon_size, double scale_factor) const {
    QVariantMap font_options;
    font_options.insert(FontIconOption::kScaleFactorAttr, QVariant::fromValue(scale_factor));
    font_options.insert(FontIconOption::kColorAttr, QColor(250, 88, 106));
    font_options.insert(FontIconOption::kSelectedColorAttr, QColor(250, 88, 106));

    auto icon = qFontIcon.getIcon(static_cast<int32_t>(Glyphs::ICON_PLAY_LIST_PAUSE), font_options);
    icon.addPixmap(icon.pixmap(icon_size, QIcon::Normal, QIcon::Off),
        QIcon::Selected, QIcon::Off);
    icon.addPixmap(icon.pixmap(icon_size, QIcon::Normal, QIcon::On),
        QIcon::Selected, QIcon::On);
    return icon;
}

QIcon ThemeManager::playlistPlayingIcon(QSize icon_size, double scale_factor) const {
    QVariantMap font_options;
    font_options.insert(FontIconOption::kScaleFactorAttr, QVariant::fromValue(scale_factor));
    font_options.insert(FontIconOption::kColorAttr, QColor(250, 88, 106));
    font_options.insert(FontIconOption::kSelectedColorAttr, QColor(250, 88, 106));
    auto icon = qFontIcon.getIcon(static_cast<int32_t>(Glyphs::ICON_PLAY_LIST_PLAY), font_options);

    icon.addPixmap(icon.pixmap(icon_size, QIcon::Normal, QIcon::Off),
        QIcon::Selected, QIcon::Off);
    icon.addPixmap(icon.pixmap(icon_size, QIcon::Normal, QIcon::On),
        QIcon::Selected, QIcon::On);
    return icon;
}

QIcon ThemeManager::playingIcon() const {
    QVariantMap font_options;
    font_options.insert(FontIconOption::kColorAttr, QColor(252, 215, 75));
    return qFontIcon.getIcon(static_cast<int32_t>(Glyphs::ICON_PLAYING), font_options);
}

QIcon ThemeManager::hdIcon() const {
    QVariantMap font_options;
    font_options.insert(FontIconOption::kColorAttr, QColor(252, 215, 75));
    return qFontIcon.getIcon(static_cast<int32_t>(Glyphs::ICON_HD_AUDIO), font_options);
}

QIcon ThemeManager::cloudIcon() const {
    QVariantMap font_options;
    font_options.insert(FontIconOption::kColorAttr, QColor(255, 255, 255));
    return qFontIcon.getIcon(static_cast<int32_t>(Glyphs::ICON_CLOUD), font_options);
}

QPixmap ThemeManager::githubIcon() const {
    if (themeColor() == ThemeColor::DARK_THEME) {
        return QPixmap(":/xamp/Resource/Black/GitHub-Mark.png"_str);
	} else {
        return QPixmap(":/xamp/Resource/White/GitHub-Mark.png"_str);
	}
}

const QPixmap& ThemeManager::unknownCover() {
    return unknown_cover_;
}

const QPixmap& ThemeManager::defaultSizeUnknownCover() {
    return default_size_unknown_cover_;
}

void ThemeManager::updateMaximumIcon(QToolButton *maxWinButton, bool is_maximum) {
    if (is_maximum) {
        maxWinButton->setIcon(fontIcon(Glyphs::ICON_RESTORE_WINDOW));
    } else {
        maxWinButton->setIcon(fontIcon(Glyphs::ICON_MAXIMUM_WINDOW));
    }
}

void ThemeManager::setHeartButton(QToolButton* heartButton, bool press) {
    heartButton->setIcon(fontIcon(press ? Glyphs::ICON_HEART_PRESS : Glyphs::ICON_HEART));
    heartButton->setStyleSheet("background: transparent;"_str);
}

void ThemeManager::setPlayOrPauseButton(QToolButton *playButton, bool is_playing) {
    if (is_playing) {
        playButton->setIcon(fontIcon(Glyphs::ICON_PAUSE));
    }
    else {
        playButton->setIcon(fontIcon(Glyphs::ICON_PLAY));
    }
}

QSize ThemeManager::titleButtonIconSize() {
    return {titleBarIconHeight(), titleBarIconHeight()};
}

void ThemeManager::setRecordIcon(QToolButton* record_button, bool is_recording) {
    if (!is_recording) {
        record_button->setIcon(fontIcon(Glyphs::ICON_MIC));
    }
    else {
        record_button->setIcon(fontIcon(Glyphs::ICON_SEND));
    }
}

void ThemeManager::setCancelRecordIcon(QToolButton* cancel_button) {
    cancel_button->setIcon(fontIcon(Glyphs::ICON_CLOSE_WINDOW));
}

Glyphs ThemeManager::connectTypeGlyphs(DeviceConnectType type) const {
    switch (type) {
    case DeviceConnectType::UNKNOWN:
    case DeviceConnectType::BUILT_IN_SPEAKER:
        return Glyphs::ICON_BUILD_IN_SPEAKER;
    case DeviceConnectType::USB:
        return Glyphs::ICON_USB;
    case DeviceConnectType::BLUE_TOOTH:
        return Glyphs::ICON_BLUE_TOOTH;
    }
    return Glyphs::ICON_BUILD_IN_SPEAKER;
}

QIcon ThemeManager::connectTypeIcon(DeviceConnectType type) {
    switch (type) {
    case DeviceConnectType::UNKNOWN:
    case DeviceConnectType::BUILT_IN_SPEAKER:
        return fontIcon(Glyphs::ICON_BUILD_IN_SPEAKER);
    case DeviceConnectType::USB:
        return fontIcon(Glyphs::ICON_USB);
    case DeviceConnectType::BLUE_TOOTH:
        return fontIcon(Glyphs::ICON_BLUE_TOOTH);
    }
    return fontIcon(Glyphs::ICON_BUILD_IN_SPEAKER);
}

void ThemeManager::setDeviceConnectTypeIcon(QAbstractButton* button, DeviceConnectType type) {
    button->setIcon(connectTypeIcon(type));
    button->update();
}

