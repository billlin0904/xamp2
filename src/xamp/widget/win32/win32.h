//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QWidget>
#include <QIcon>

class IXPlayerFrame;
class XWindow;
class QWinThumbnailToolBar;
class QWinTaskbarButton;
class QWinTaskbarProgress;

namespace win32 {

class WinTaskbar {
public:
    WinTaskbar(XWindow* window, IXPlayerFrame* player_frame);

    ~WinTaskbar();

    void setTaskbarProgress(const int32_t percent);

    void resetTaskbarProgress();

    void setTaskbarPlayingResume();

    void setTaskbarPlayerPaused();

    void setTaskbarPlayerPlaying();

    void setTaskbarPlayerStop();

    void showEvent();

    QIcon play_icon;
    QIcon pause_icon;
    QIcon stop_play_icon;
    QIcon seek_forward_icon;
    QIcon seek_backward_icon;

private:
    XWindow* window_;
    QScopedPointer<QWinThumbnailToolBar> thumbnail_tool_bar_;
    QScopedPointer<QWinTaskbarButton> taskbar_button_;
    QWinTaskbarProgress* taskbar_progress_;
};

void setAccentPolicy(const WId window_id, bool enable = true, int animation_id = 0);
void setFramelessWindowStyle(const WId window_id);
void setWindowedWindowStyle(const WId window_id);
void setTitleBarColor(const WId window_id, QColor color);
void frameChange(const WId window_id);
void addDwmShadow(const WId window_id);
void addDwmMenuShadow(const WId window_id);
void removeStandardFrame(void* message);
void setResizeable(void* message);
bool isWindowMaximized(const WId window_id);
bool compositionEnabled();
QRect getWindowRect(const WId window_id);

}

