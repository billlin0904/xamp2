//=====================================================================================================================
// Copyright (c) 2018-2020 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QWidget>
#include <QIcon>
#include <QAbstractNativeEventFilter>

#include "thememanager.h"

class IXFrame;
class XMainWindow;
class QWinThumbnailToolBar;
class QWinTaskbarButton;
class QWinTaskbarProgress;

namespace win32 {

class WinTaskbar {
public:
    WinTaskbar(XMainWindow* window, IXFrame* player_frame);

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
    XMainWindow* window_;
    QScopedPointer<QWinThumbnailToolBar> thumbnail_tool_bar_;
    QScopedPointer<QWinTaskbarButton> taskbar_button_;
    QWinTaskbarProgress* taskbar_progress_;
};

void setAccentPolicy(const WId window_id, bool enable = true, int animation_id = 0) noexcept;
void setFramelessWindowStyle(const WId window_id) noexcept;
void setWindowedWindowStyle(const WId window_id) noexcept;
void setTitleBarColor(const WId window_id, ThemeColor theme_color) noexcept;
void addDwmShadow(const WId window_id) noexcept;
void addDwmMenuShadow(const WId window_id) noexcept;
bool isWindowMaximized(const WId window_id) noexcept;
bool compositionEnabled() noexcept;
QRect windowRect(const WId window_id) noexcept;
QColor colorizationColor() noexcept;
bool isDarkModeAppEnabled() noexcept;
std::string getRandomMutexName(const std::string& src_name);
bool isRunning(const std::string& mutex_name);

}

