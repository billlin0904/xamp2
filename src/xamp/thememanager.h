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
    virtual const QIcon& volumeUp() const noexcept = 0;
    virtual const QIcon& volumeOff() const noexcept = 0;
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

    const QIcon& volumeUp() const noexcept override;

    const QIcon& volumeOff() const noexcept override;
private:
    QPixmap unknown_cover_;
    QPixmap default_size_unknown_cover_;
    QIcon volume_up_;
    QIcon volume_off_;
};

class ThemeManager {
public:
    static const StylePixmapManager& pixmap() noexcept;
    static void setPlayOrPauseButton(Ui::XampWindow &ui, bool is_playing);
    static void setDefaultStyle(Ui::XampWindow &ui);
    static void setNightStyle(Ui::XampWindow &ui);
    static QString getMenuStyle() noexcept;
    static QSize getDefaultCoverSize() noexcept;
};

