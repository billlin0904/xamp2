//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QPixmap>
#include <QIcon>

namespace Ui {
class XampWindow;
}

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
    WHITE_THEME,
};

class ThemeManager : public QObject {
    Q_OBJECT
public:    
    static ThemeManager& instance();

    QIcon volumeUp();

    QIcon volumeOff();

    const StylePixmapManager& pixmap() noexcept;

    void setPlayOrPauseButton(Ui::XampWindow &ui, bool is_playing);

    void setDefaultStyle(Ui::XampWindow &ui);

    void setBackgroundColor(Ui::XampWindow& ui, QColor backgroundColor);    

    void enableBlur(const QWidget* widget, bool enable);

    QString getMenuStyle() noexcept;

    QSize getDefaultCoverSize() noexcept;

    QSize getCacheCoverSize() noexcept;

    QSize getAlbumCoverSize() noexcept;

    QColor getBackgroundColor() noexcept;

    void setThemeIcon(Ui::XampWindow& ui);

    QIcon playArrow() noexcept;

    void setShufflePlayorder(Ui::XampWindow& ui);

    void setRepeatOnePlayorder(Ui::XampWindow& ui);

    void setRepeatOncePlayorder(Ui::XampWindow& ui);

    void setThemeColor(ThemeColor theme_color);

signals:
    void themeChanged(ThemeColor theme_color);    

private:
    QLatin1String themeColorPath() const;

    ThemeManager();

    ThemeColor theme_color_;
    QSize defaultAlbumCoverSize;
    QSize defaultCoverSize;
    QColor tableTextColor;
    QColor menuColor;
    QColor menuTextColor;
    QColor backgroundColor;
    QColor controlBackgroundColor;
};

