//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <xampplayer.h>
#include <thememanager.h>

#include <widget/driveinfo.h>

#if defined(Q_OS_WIN)
class WinTaskbar;
#endif

class XAMP_WIDGET_SHARED_EXPORT XMainWindow final : public IXMainWindow {
public:
    XMainWindow();

	virtual ~XMainWindow() override;

    void setShortcut(const QKeySequence& shortcut) override;

    void setContentWidget(IXFrame *content_widget);

    void setTaskbarProgress(int32_t percent) override;

    void resetTaskbarProgress() override;

    void setTaskbarPlayingResume() override;

    void setTaskbarPlayerPaused() override;

    void setTaskbarPlayerPlaying() override;

    void setTaskbarPlayerStop() override;

    void restoreAppGeometry() override;

    void initMaximumState() override;

    void updateMaximumState() override;

    void setIconicThumbnail(const QPixmap& image) override;

    void readDriveInfo();

    void drivesRemoved(char driver_letter);

    void systemThemeChanged(ThemeColor theme_color);

    void shortcutsPressed(uint16_t native_key, uint16_t native_mods);

    void showWindow();

protected:
    bool eventFilter(QObject* object, QEvent* event) override;

    void dragEnterEvent(QDragEnterEvent *event) override;

    void dragMoveEvent(QDragMoveEvent *event) override;

    void dragLeaveEvent(QDragLeaveEvent *event) override;

    void dropEvent(QDropEvent *event) override;

    void closeEvent(QCloseEvent* event) override;

    void saveAppGeometry() override;

private:
    bool nativeEvent(const QByteArray& event_type, void* message, qintptr* result) override;

    void showEvent(QShowEvent* event) override;

    void ensureInitTaskbar();

    uint32_t screen_number_;
    QPoint last_pos_;
#if defined(Q_OS_WIN)
    QScopedPointer<WinTaskbar> task_bar_;
    QMap<QString, DriveInfo> exist_drives_;
#endif
    QMap<QPair<quint32, quint32>, QKeySequence>  shortcuts_;
	IXFrame *content_widget_;
};
