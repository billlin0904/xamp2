//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QAbstractNativeEventFilter>

#include <atlcomcli.h>
#include <shobjidl_core.h>

#include <thememanager.h>

class IXFrame;
class XMainWindow;

struct ITaskbarList4;

enum TaskbarProgressState {
    TASKBAR_PROCESS_STATE_NO_PROCESS, // Hidden
    TASKBAR_PROCESS_STATE_INDETERMINATE, // Busy
    TASKBAR_PROCESS_STATE_NORMAL,
    TASKBAR_PROCESS_STATE_ERROR, // Stopped
    TASKBAR_PROCESS_STATE_PAUSED,
};

constexpr auto kWinThumbbarButtonSize = 4;

class WinTaskbar : public QObject, public QAbstractNativeEventFilter {
public:
    WinTaskbar(XMainWindow* window, IXFrame *frame);

    virtual ~WinTaskbar() override;

    void setTheme(ThemeColor theme_color);

    void setTaskbarProgress(const int32_t process);

    void setIconicThumbnail(const QPixmap& image);

    void resetTaskbarProgress();

    void setTaskbarPlayingResume();

    void setTaskbarPlayerPaused();

    void setTaskbarPlayerPlaying();

    void setTaskbarPlayerStop();

    void updateProgressIndicator();

    void updateOverlay();

    void setRange(int progress_minimum, int progress_maximum);

private:
    bool nativeEventFilter(const QByteArray& event_type, void* message, qintptr* result) override;

    void updateThumbnailButton(UINT iId, HICON hIcon, LPCWSTR tooltip);

    void addThumbnailButtons();

    void setWindow(QWidget* window);

    struct ButtonIcon;

    TaskbarProgressState state_{ TaskbarProgressState::TASKBAR_PROCESS_STATE_NO_PROCESS };
    QIcon play_icon;
    QIcon pause_icon;
    QIcon stop_play_icon;
    QIcon seek_forward_icon;
    QIcon seek_backward_icon;
    int32_t process_max_{ 0 };
    int32_t process_min_{ 0 };
    int32_t process_value_{ 0 };
    QWidget* window_{ nullptr };
    IXFrame* frame_{ nullptr };
    QIcon overlay_icon_;
    QString overlay_accessible_description_;
    QPixmap thumbnail_;
    CComPtr<ITaskbarList4> taskbar_list_;
	ScopedPtr<ButtonIcon> button_icons_;
};

