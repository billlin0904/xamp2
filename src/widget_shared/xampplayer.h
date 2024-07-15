//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================
#pragma once

#include <QWidget>
#include <QFrame>
#include <QMainWindow>

#include <thememanager.h>
#include <widget/driveinfo.h>
#include <FramelessHelper/Widgets/framelesswidget.h>
#include <FramelessHelper/Widgets/framelessmainwindow.h>

inline constexpr auto kRestartExistCode = -2;

FRAMELESSHELPER_USE_NAMESPACE
using namespace wangwenx190::FramelessHelper::Global;

class IXFrame;

#ifdef Q_OS_WIN
using IXMainWindowBase = FramelessWidget;
#else
using IXMainWindowBase = QWidget;
#endif

class IXMainWindow : public IXMainWindowBase {
public:
	virtual ~IXMainWindow() override = default;

    virtual void setShortcut(const QKeySequence& shortcut) = 0;

    virtual void setTaskbarProgress(int32_t percent) = 0;

    virtual void resetTaskbarProgress() = 0;

    virtual void setTaskbarPlayingResume() = 0;

    virtual void setTaskbarPlayerPaused() = 0;

    virtual void setTaskbarPlayerPlaying() = 0;

    virtual void setTaskbarPlayerStop() = 0;

    virtual void initMaximumState() = 0;

    virtual void updateMaximumState() = 0;

    virtual void saveAppGeometry() = 0;

    virtual void restoreAppGeometry() = 0;

    virtual void setIconicThumbnail(const QPixmap& image) = 0;

    virtual IXFrame* contentWidget() const = 0;

    virtual void showWindow() = 0;
protected:
    IXMainWindow() = default;
};

class IXFrame : public QFrame {
public:
    virtual ~IXFrame() override = default;

    virtual void addDropFileItem(const QUrl& url) = 0;

    virtual void playPrevious() = 0;

    virtual void playNext() = 0;

    virtual void stopPlay() = 0;

    virtual void playOrPause() = 0;

    virtual void drivesChanges(const QList<DriveInfo>& drive_infos) = 0;

    virtual void drivesRemoved(const DriveInfo& drive_info) = 0;

    virtual void updateMaximumState(bool is_maximum) = 0;

    virtual void shortcutsPressed(const QKeySequence& shortcut) = 0;

    virtual QString translateText(const std::string_view& text) = 0;
protected:
    explicit IXFrame(QWidget* parent = nullptr)
	    : QFrame(parent) {
    }
};

