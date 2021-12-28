//=====================================================================================================================
// Copyright (c) 2018-2021 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QPixmap>
#include <QIcon>
#include <QMenu>

#include <widget/str_utilts.h>

namespace Ui {
class XampWindow;
}

inline QString colorToString(QColor color) noexcept {
    return QString(Q_UTF8("rgba(%1,%2,%3,%4)"))
        .arg(color.red())
        .arg(color.green())
        .arg(color.blue())
        .arg(color.alpha());
}

inline QString backgroundColorToString(QColor color) noexcept {
    return Q_UTF8("background-color: ") + colorToString(color) + Q_UTF8(";");
}

inline constexpr int32_t kUIRadius = 9;

class StylePixmapManager {
public:
    virtual ~StylePixmapManager() = default;
    virtual const QPixmap& unknownCover() const noexcept = 0;
    virtual const QPixmap& defaultSizeUnknownCover() const noexcept = 0;
protected:
    StylePixmapManager() = default;
};

class DefaultStylePixmapManager : public StylePixmapManager {
public:
    DefaultStylePixmapManager();

    ~DefaultStylePixmapManager() override = default;

    const QPixmap& unknownCover() const noexcept override;

    const QPixmap& defaultSizeUnknownCover() const noexcept override;
private:
    QPixmap unknown_cover_;
    QPixmap default_size_unknown_cover_;
};

enum class ThemeColor {
    DARK_THEME,
    LIGHT_THEME,
};

class ThemeManager : public QObject {
    Q_OBJECT
public:    
    static ThemeManager& instance();

    const QPalette& palette() const {
        return palette_;
    }

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

    const StylePixmapManager& pixmap() noexcept;

    void setPlayOrPauseButton(Ui::XampWindow &ui, bool is_playing);

    void setWidgetStyle(Ui::XampWindow &ui);    

    void enableBlur(const QWidget* widget, bool enable) const;

    QSize getDefaultCoverSize() const noexcept;

    QSize getCacheCoverSize() const noexcept;

    QSize getAlbumCoverSize() const noexcept;

    QColor getBackgroundColor() const noexcept;    

    QIcon playArrow() const noexcept;

    QIcon speaker() const;

    QIcon usb() const;

    QIcon ytMusicIcon() const;

    void setThemeIcon(Ui::XampWindow& ui) const;

    void setShufflePlayorder(Ui::XampWindow& ui) const;

    void setRepeatOnePlayOrder(Ui::XampWindow& ui) const;

    void setRepeatOncePlayOrder(Ui::XampWindow& ui) const;

    void setThemeColor(ThemeColor theme_color);

    void setMenuStlye(QMenu *menu) const;

    ThemeColor themeColor() const {
        return theme_color_;
    }

    void setBackgroundColor(Ui::XampWindow& ui, QColor color);

    void setBackgroundColor(QWidget* widget, int32_t alpha = 255);

private:
    void setPalette();

    QIcon makeIcon(const QString& path) const;

    QLatin1String themeColorPath() const;

    ThemeManager();

    ThemeColor theme_color_;
    QSize album_cover_size_;
    QSize cover_size_;
    QColor background_color_;
    QPalette palette_;
};

