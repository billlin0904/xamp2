//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include "xampplayer.h"
#include <thememanager.h>

#include <widget/driveinfo.h>

#if defined(Q_OS_WIN)
class QScreen;
namespace win32 {
    class WinTaskbar;
}
#endif

class XAMP_WIDGET_SHARED_EXPORT XMainWindow final : public IXMainWindow {
public:
    XMainWindow();

	virtual ~XMainWindow() override;

    void SetShortcut(const QKeySequence& shortcut) override;

    void SetContentWidget(IXFrame *content_widget);

    void SetTaskbarProgress(int32_t percent) override;

    void ResetTaskbarProgress() override;

    void SetTaskbarPlayingResume() override;

    void SetTaskbarPlayerPaused() override;

    void SetTaskbarPlayerPlaying() override;

    void SetTaskbarPlayerStop() override;

    void RestoreGeometry() override;

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

    void changeEvent(QEvent * event) override;

    void closeEvent(QCloseEvent* event) override;

    void SaveGeometry() override;

private:
    void AddSystemMenu(QWidget *widget);

    void focusInEvent(QFocusEvent* event) override;

    void focusOutEvent(QFocusEvent* event) override;

    void showEvent(QShowEvent* event) override;

    uint32_t screen_number_;
    QPoint last_pos_;
    QRect last_rect_;
#if defined(Q_OS_WIN)
    QScreen* current_screen_;
    QScopedPointer<win32::WinTaskbar> task_bar_;
    QMap<QString, DriveInfo> exist_drives_;
#endif
    QMap<QPair<quint32, quint32>, QKeySequence>  shortcuts_;
	IXFrame *content_widget_;
};
