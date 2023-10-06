//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <atlcomcli.h>
#include <shobjidl_core.h>

#include <QEvent>
#include <QAbstractNativeEventFilter>

#include <thememanager.h>

class IXFrame;
class XMainWindow;

class ITaskbarList4;

namespace win32 {

 enum TaskbarProgressState {
    TASKBAR_PROCESS_STATE_NO_PROCESS, // Hidden
    TASKBAR_PROCESS_STATE_INDETERMINATE, // Busy
    TASKBAR_PROCESS_STATE_NORMAL,
    TASKBAR_PROCESS_STATE_ERROR, // Stopped
    TASKBAR_PROCESS_STATE_PAUSED,
 };

 constexpr auto kWinThumbbarButtonSize = 3;

class WinTaskbar : public QObject, public QAbstractNativeEventFilter {
    Q_OBJECT
public:    
    WinTaskbar(XMainWindow* window, IXFrame* player_frame);

    virtual ~WinTaskbar() override;

    void SetTaskbarProgress(const int32_t process);

    void SetIconicThumbnail(const QPixmap &image);

    void ResetTaskbarProgress();

    void SetTaskbarPlayingResume();

    void SetTaskbarPlayerPaused();

    void SetTaskbarPlayerPlaying();

    void SetTaskbarPlayerStop();

    void UpdateProgressIndicator();

    void UpdateOverlay();

    void SetRange(int progress_minimum, int progress_maximum);

    bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override;

    QIcon play_icon;
    QIcon pause_icon;
    QIcon stop_play_icon;
    QIcon seek_forward_icon;
    QIcon seek_backward_icon;
    std::array<THUMBBUTTON, kWinThumbbarButtonSize> buttons;

signals:
    void PlayClicked();

    void PauseClicked();

    void ForwardClicked();

    void BackwardClicked();

private:    
    void CreateToolbarImages();

    void InitialToolbarButtons();

    void SetWindow(QWidget* window);

    int process_max_{0};
    int process_min_{0};
    int process_value_{0};
    QWidget* window_{ nullptr };
    QIcon overlay_icon_;    
    TaskbarProgressState state_;
    QString overlay_accessible_description_;
    QPixmap thumbnail_;
    CComPtr<ITaskbarList4> taskbar_list_;
};

}
