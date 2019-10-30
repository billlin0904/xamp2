//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
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
protected:
    StylePixmapManager() = default;
};

class ThemeManager {
public:
    static const StylePixmapManager& pixmap();
    static void setDefaultStyle(Ui::XampWindow &ui);
    static void setNightStyle(Ui::XampWindow &ui);
};

