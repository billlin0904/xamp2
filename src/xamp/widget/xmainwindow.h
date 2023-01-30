//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include "xampplayer.h"
#include "thememanager.h"

#include <widget/driveinfo.h>

#if defined(Q_OS_WIN)
class QScreen;
namespace win32 {
    class WinTaskbar;
}
#endif

class XMainWindow final : public IXMainWindow {
public:
    XMainWindow();

	virtual ~XMainWindow() override;

    void setShortcut(const QKeySequence& shortcut) override;

    void SetContentWidget(IXFrame *content_widget);

    void SetTaskbarProgress(int32_t percent) override;

    void ResetTaskbarProgress() override;

    void SetTaskbarPlayingResume() override;

    void SetTaskbarPlayerPaused() override;

    void SetTaskbarPlayerPlaying() override;

    void SetTaskbarPlayerStop() override;

    void SetTitleBarAction(QFrame* title_bar) override;

    void RestoreGeometry();

    void InitMaximumState() override;

    void UpdateMaximumState() override;

    void ReadDriveInfo();

    void DrivesRemoved(char driver_letter);

    void SystemThemeChanged(ThemeColor theme_color);

    void ShortcutsPressed(uint16_t native_key, uint16_t native_mods);

    void ShowWindow();
protected:
    bool eventFilter(QObject* object, QEvent* event) override;

    void dragEnterEvent(QDragEnterEvent *event) override;

    void dragMoveEvent(QDragMoveEvent *event) override;

    void dragLeaveEvent(QDragLeaveEvent *event) override;

    void dropEvent(QDropEvent *event) override;

    void mousePressEvent(QMouseEvent *event) override;

    void mouseReleaseEvent(QMouseEvent * event) override;

    void mouseMoveEvent(QMouseEvent * event) override;

    void mouseDoubleClickEvent(QMouseEvent* event) override;

    void changeEvent(QEvent * event) override;

    void closeEvent(QCloseEvent* event) override;

    void SaveGeometry() override;

    bool nativeEvent(const QByteArray& event_type, void* message, long* result) override;
private:
    void AddSystemMenu(QWidget *widget);

    void focusInEvent(QFocusEvent* event) override;

    void focusOutEvent(QFocusEvent* event) override;

    void showEvent(QShowEvent* event) override;

    void UpdateScreenNumber();

    QPoint last_pos_;
    QRect last_rect_;
    uint32_t screen_number_;

#if defined(Q_OS_WIN)
    QScreen* current_screen_;
    QScopedPointer<win32::WinTaskbar> taskbar_;
    QMap<QString, DriveInfo> exist_drives_;
#endif
    QMap<QPair<quint32, quint32>, QKeySequence>  shortcuts_;
    IXFrame *player_control_frame_;
};
