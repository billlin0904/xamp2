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

namespace Ui {
class XampWindow;
}

inline constexpr int32_t kUIRadius = 9;

enum class ThemeColor {
    DARK_THEME,
    LIGHT_THEME,
};

class ThemeManager {
public:
    friend class SharedSingleton<ThemeManager>;

    bool useNativeWindow() const;

    bool enableBlur() const;

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

    void setPlayOrPauseButton(Ui::XampWindow &ui, bool is_playing);

    void setBitPerfectButton(Ui::XampWindow& ui, bool enable);

    void setWidgetStyle(Ui::XampWindow &ui);    

    void enableBlur(QWidget* widget, bool enable = true) const;

    const QSize& getDefaultCoverSize() const noexcept;

    QSize getCacheCoverSize() const noexcept;

    QSize getAlbumCoverSize() const noexcept;

    QSize getSaveCoverArtSize() const noexcept;

    QColor getBackgroundColor() const noexcept;    

    QIcon playArrow() const noexcept;

    QIcon playIcon() const noexcept;

    QIcon speakerIcon() const;

    QIcon usbIcon() const;

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

    QString backgroundColor() const;

    QColor hoverColor() const;

    QColor titleBarColor() const;

private:
    QFont loadFonts();

    void installFont(const QString& file_name, QList<QString>& ui_fallback_fonts);

    void setPalette();

    QIcon makeIcon(const QString& path) const;

    ThemeColor themeColor() const {
        return theme_color_;
    }

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

