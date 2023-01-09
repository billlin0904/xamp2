//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================
#pragma once

#include <QWidget>
#include <QFrame>

#include "thememanager.h"
#include <widget/driveinfo.h>

inline constexpr auto kRestartExistCode = -2;

class IXWindow : public QFrame {
public:
    virtual ~IXWindow() override = default;

    virtual void setShortcut(const QKeySequence& shortcut) = 0;

    virtual void setTaskbarProgress(int32_t percent) = 0;

    virtual void resetTaskbarProgress() = 0;

    virtual void setTaskbarPlayingResume() = 0;

    virtual void setTaskbarPlayerPaused() = 0;

    virtual void setTaskbarPlayerPlaying() = 0;

    virtual void setTaskbarPlayerStop() = 0;

    virtual void setTitleBarAction(QFrame *title_bar) = 0;

    virtual void initMaximumState() = 0;

    virtual void updateMaximumState() = 0;

    virtual void saveGeometry() = 0;
protected:
    IXWindow() = default;
};

class IXPlayerControlFrame : public QFrame {
public:
    virtual ~IXPlayerControlFrame() override = default;

    virtual void addDropFileItem(const QUrl& url) = 0;

    virtual void deleteKeyPress() = 0;

    virtual void playPrevious() = 0;

    virtual void playNext() = 0;

    virtual void stop() = 0;

    virtual void playOrPause() = 0;

    virtual bool hitTitleBar(const QPoint &ps) const = 0;

    virtual void drivesChanges(const QList<DriveInfo>& drive_infos) = 0;

    virtual void drivesRemoved(const DriveInfo& drive_info) = 0;

    virtual void updateMaximumState(bool is_maximum) = 0;

    virtual void focusIn() = 0;

    virtual void focusOut() = 0;

    virtual void systemThemeChanged(ThemeColor theme_color) = 0;

    virtual void shortcutsPressed(const QKeySequence& shortcut) = 0;
protected:
    IXPlayerControlFrame() = default;
};

