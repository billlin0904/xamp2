//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================
#pragma once

#include <QWidget>
#include <QFrame>
#include <QMainWindow>

#include "thememanager.h"
#include <widget/driveinfo.h>

inline constexpr auto kRestartExistCode = -2;

class IXMainWindow : public QFrame {
public:
    virtual ~IXMainWindow() override = default;

    virtual void setShortcut(const QKeySequence& shortcut) = 0;

    virtual void SetTaskbarProgress(int32_t percent) = 0;

    virtual void ResetTaskbarProgress() = 0;

    virtual void SetTaskbarPlayingResume() = 0;

    virtual void SetTaskbarPlayerPaused() = 0;

    virtual void SetTaskbarPlayerPlaying() = 0;

    virtual void SetTaskbarPlayerStop() = 0;

    virtual void SetTitleBarAction(QFrame *title_bar) = 0;

    virtual void InitMaximumState() = 0;

    virtual void UpdateMaximumState() = 0;

    virtual void SaveGeometry() = 0;

    virtual void RestoreGeometry() = 0;
protected:
    IXMainWindow() = default;
};

class IXFrame : public QFrame {
public:
    virtual ~IXFrame() override = default;

    virtual void AddDropFileItem(const QUrl& url) = 0;

    virtual void DeleteKeyPress() = 0;

    virtual void PlayPrevious() = 0;

    virtual void PlayNext() = 0;

    virtual void stop() = 0;

    virtual void PlayOrPause() = 0;

    virtual bool HitTitleBar(const QPoint &ps) const = 0;

    virtual void DrivesChanges(const QList<DriveInfo>& drive_infos) = 0;

    virtual void DrivesRemoved(const DriveInfo& drive_info) = 0;

    virtual void UpdateMaximumState(bool is_maximum) = 0;

    virtual void FocusIn() = 0;

    virtual void FocusOut() = 0;

    virtual void ShortcutsPressed(const QKeySequence& shortcut) = 0;
protected:
    IXFrame() = default;
};

