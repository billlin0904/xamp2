//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QPixmap>
#include <QIcon>
#include <QMenu>
#include <QFrame>

#include <widget/widget_shared.h>
#include <widget/str_utilts.h>
#include <widget/fonticon.h>

class QPushButton;
class QToolButton;

namespace Ui {
class XampWindow;
}

inline constexpr int32_t kUIRadius = 9;

enum class ThemeColor {
    DARK_THEME,
    LIGHT_THEME,
};

enum IconCode {
    ICON_VolumeUp = 0xE995,
    ICON_VolumeOff = 0xE74F,
    ICON_Speaker = 0xE7F5,
    ICON_Folder = 0xF12B,
    ICON_LoadFile = 0xE8E5,
    ICON_LoadDir = 0xE838,
    ICON_Reload = 0xE895,
    ICON_RemoveAll = 0xE894,
    ICON_OpenFilePath = 0xE8DA,
    ICON_ReadReplayGain = 0xe023,
    ICON_ExportFile = 0xE78C,
    ICON_Copy = 0xE8C8,
    ICON_Download = 0xE896,
    ICON_Playlist = 0xE90B,
    ICON_Equalizer = 0xe9e9,
    ICON_Podcast = 0xEFA9,
    ICON_Album = 0xE93C,
    ICON_CD = 0xE958,
    ICON_LeftArrow = 0xEC52,
    ICON_Artist = 0xE716,
    ICON_Subtitle = 0xED1E,
    ICON_Preference = 0xF8A6,
    ICON_About = 0xF167,
    ICON_DarkMode = 0xEC46,
    ICON_LightMode = 0xF08C,
    ICON_Search = 0xF78B,
    ICON_Theme = 0xE771,
    ICON_Desktop = 0xEC4E,
    ICON_ShufflePlayOrder = 0xE8B1,
    ICON_RepeatOnePlayOrder = 0xE8ED,
    ICON_RepeatOncePlayOrder = 0xE8EE,
    ICON_MinimizeWindow = 0xE921,
    ICON_MaximumWindow = 0xE922,
    ICON_CloseWindow = 0xE8BB,
    ICON_RestoreWindow = 0xE923,
    ICON_SliderBar = 0xE700,
    ICON_Play = 0xF5B0,
    ICON_Pause = 0xF8AE,
    ICON_Stop = 0xE978,
    ICON_PlayNext = 0xF8AD,
    ICON_PlayPrev = 0xF8AC,
    ICON_More = 0xE712
};

//enum IconCode {
//    ICON_VolumeUp = 0xe050,
//    ICON_VolumeOff = 0xe04f,
//    ICON_Speaker = 0xe32d,
//    ICON_Folder = 0xe2c7,
//    ICON_LoadFile = 0xe89c,
//    ICON_LoadDir = 0xe2cc,
//    ICON_Reload = 0xe5d5,
//    ICON_RemoveAll = 0xeb80,
//    ICON_OpenFilePath = 0xe880,
//    ICON_ReadReplayGain = 0xe023,
//    ICON_ExportFile = 0xe0c3,
//    ICON_Copy = 0xe14d,
//    ICON_Download = 0xe2c4,
//    ICON_Playlist = 0xe03d,
//    ICON_Equalizer = 0xe01d,
//    ICON_Podcast = 0xf048,
//    ICON_Album = 0xe030,
//    ICON_CD = 0xe019,
//    ICON_Artist = 0xe7ef,
//    ICON_Subtitle = 0xec0b,
//    ICON_Preference = 0xe8b8,
//    ICON_About = 0xe887,
//    ICON_DarkMode = 0xe51c,
//    ICON_LightMode = 0xe518,
//    ICON_Search = 0xe8b6,
//    ICON_Theme = 0xe40a,
//    ICON_Desktop = 0xe30a,
//    ICON_ShufflePlayOrder = 0xe043,
//    ICON_RepeatOnePlayOrder = 0xe040,
//    ICON_RepeatOncePlayOrder = 0xe041,
//    ICON_MinimizeWindow = 0xE921,
//    ICON_MaximumWindow = 0xE922,
//    ICON_CloseWindow = 0xE8BB,
//    ICON_RestoreWindow = 0xE923,
//    ICON_SliderBar = 0xe5d2,
//    ICON_Play = 0xe1a2,
//    ICON_Pause = 0xe1c4,
//    ICON_PlayNext = 0xe044,
//    ICON_Stop = 0xe047,
//    ICON_PlayPrev = 0xe045,
//    ICON_More = 0xe5d4
//};

class ThemeManager {
public:
    friend class SharedSingleton<ThemeManager>;

    bool useNativeWindow() const;

    const QPalette& palette() const {
        return palette_;
    }

    const QFont& defaultFont() const {
        return ui_font_;
    }

    const QPixmap& unknownCover() const noexcept;

	const QPixmap& defaultSizeUnknownCover() const noexcept;

    QIcon desktopIcon() const;

    QIcon folderIcon() const;

    QIcon appIcon() const;

    QIcon volumeUp() const;

    QIcon volumeOff() const;

    QIcon playlistIcon() const;

    QIcon podcastIcon() const;

    QIcon equalizerIcon() const;

    QIcon albumsIcon() const;

    QIcon artistsIcon() const;

    QIcon subtitleIcon() const;

    QIcon preferenceIcon() const;

    QIcon aboutIcon() const;

    QIcon darkModeIcon() const;

    QIcon lightModeIcon() const;

    QIcon seachIcon() const;

    QIcon themeIcon() const;

    QIcon cdIcon() const;

    QIcon moreIcon() const;

    void setPlayOrPauseButton(Ui::XampWindow &ui, bool is_playing);

    void setBitPerfectButton(Ui::XampWindow& ui, bool enable);

    void setWidgetStyle(Ui::XampWindow &ui);    

    const QSize& defaultCoverSize() const noexcept;

    QSize cacheCoverSize() const noexcept;

    QSize albumCoverSize() const noexcept;

    QSize saveCoverArtSize() const noexcept;

    QIcon playArrow() const;

    QIcon playCircleIcon() const;

    QIcon speakerIcon() const;

    QIcon minimizeWindowIcon() const;

    QIcon maximumWindowIcon() const;

    QIcon closeWindowIcon() const;

    QIcon restoreWindowIcon() const;

    QIcon sliderBarIcon() const;

    QPixmap githubIcon() const;

    void updateMaximumIcon(Ui::XampWindow& ui, bool is_maximum) const;

    void updateTitlebarState(QFrame* title_bar, bool is_focus);

    void setThemeIcon(Ui::XampWindow& ui) const;

    void setShufflePlayorder(Ui::XampWindow& ui) const;

    void setRepeatOnePlayOrder(Ui::XampWindow& ui) const;

    void setRepeatOncePlayOrder(Ui::XampWindow& ui) const;

    void setThemeColor(ThemeColor theme_color);

    void setBackgroundColor(Ui::XampWindow& ui, QColor color);

    void setThemeButtonIcon(Ui::XampWindow& ui);

    void applyTheme();

    void setBackgroundColor(QWidget* widget);

    QLatin1String themeColorPath() const;

    void setMenuStyle(QWidget* menu);

    QColor themeTextColor() const;

    QString backgroundColorString() const;

    QColor backgroundColor() const noexcept;

    QColor hoverColor() const;

    QColor titleBarColor() const;

    QColor coverShadownColor() const;

    ThemeColor themeColor() const {
        return theme_color_;
    }

    QSize tabIconSize() const;

    void setStandardButtonStyle(QToolButton* close_button, QToolButton* min_win_button, QToolButton* max_win_button) const;

    QIcon iconFromFont(const QChar& code) const;

private:
    static QString fontNamePath(const QString& file_name);

    QFont loadFonts();

    void installFileFont(const QString& file_name, QList<QString>& ui_fallback_fonts);

    void setPalette();

    QIcon makeIcon(const QString& path) const;

    ThemeManager();
    
    bool use_native_window_;
    ThemeColor theme_color_;
    QSize album_cover_size_;
    QSize cover_size_;
    QSize save_cover_art_size_;
    QColor background_color_;
    QPalette palette_;
    QFont ui_font_;
    QIcon play_arrow_;
    QPixmap unknown_cover_;
    QPixmap default_size_unknown_cover_;
};

#define qTheme SharedSingleton<ThemeManager>::GetInstance()

