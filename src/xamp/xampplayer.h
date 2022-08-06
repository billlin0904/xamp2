//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================
#pragma once

#include <QWidget>
#include <QFrame>

#include <widget/driveinfo.h>

inline constexpr auto kRestartExistCode = -2;

class IXWindow : public QFrame {
public:
    virtual ~IXWindow() override = default;

    virtual void setTaskbarProgress(int32_t percent) = 0;

    virtual void resetTaskbarProgress() = 0;

    virtual void setTaskbarPlayingResume() = 0;

    virtual void setTaskbarPlayerPaused() = 0;

    virtual void setTaskbarPlayerPlaying() = 0;

    virtual void setTaskbarPlayerStop() = 0;

    virtual void setTitleBarAction(QFrame *title_bar) = 0;

    virtual void initMaximumState() = 0;

    virtual void updateMaximumState() = 0;
protected:
    IXWindow() = default;
};

class IXPlayerFrame : public QFrame {
public:
    virtual ~IXPlayerFrame() override = default;

    virtual void addDropFileItem(const QUrl& url) = 0;

    virtual void deleteKeyPress() = 0;

    virtual void playPreviousClicked() = 0;

    virtual void playNextClicked() = 0;

    virtual void stopPlayedClicked() = 0;

    virtual void play() = 0;

    virtual bool hitTitleBar(const QPoint &ps) const = 0;

    virtual void drivesChanges(const QList<DriveInfo>& drive_infos) = 0;

    virtual void drivesRemoved(const DriveInfo& drive_info) = 0;

    virtual void updateMaximumState(bool is_maximum) = 0;

    virtual void focusInEvent() = 0;

    virtual void focusOutEvent() = 0;
protected:
    IXPlayerFrame() = default;
};

