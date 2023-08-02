//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QWidget>
#include <QIcon>

#include <thememanager.h>

class IXFrame;
class XMainWindow;
class QWinThumbnailToolBar;
class QWinTaskbarButton;
class QWinTaskbarProgress;

namespace win32 {

class WinTaskbar final {
public:
    WinTaskbar(XMainWindow* window, IXFrame* player_frame);

    ~WinTaskbar();

    void SetTaskbarProgress(const int32_t percent);

    void ResetTaskbarProgress();

    void SetTaskbarPlayingResume();

    void SetTaskbarPlayerPaused();

    void SetTaskbarPlayerPlaying();

    void SetTaskbarPlayerStop();

    void showEvent();

    QIcon play_icon;
    QIcon pause_icon;
    QIcon stop_play_icon;
    QIcon seek_forward_icon;
    QIcon seek_backward_icon;

private:
    XMainWindow* window_;
    QScopedPointer<QWinThumbnailToolBar> thumbnail_tool_bar_;
    QScopedPointer<QWinTaskbarButton> taskbar_button_;
    QWinTaskbarProgress* taskbar_progress_;
};

QRect GetWindowRect(const WId window_id);

}

