//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <xampplayer.h>
#include <thememanager.h>
#include <QSystemTrayIcon>
#include <widget/driveinfo.h>

class WinTaskbar;

class XAMP_WIDGET_SHARED_EXPORT XMainWindow final : public IXMainWindow {
public:
    static constexpr auto kMaxTitleHeight = 30;
    static constexpr auto kMaxTitleIcon = 20;
    static constexpr auto kTitleFontSize = 10;

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

    void setIconicThumbnail(const QPixmap& image) override;

    void readDriveInfo();

    void drivesRemoved(char driver_letter);

    void systemThemeChanged(ThemeColor theme_color);

    void shortcutsPressed(uint16_t native_key, uint16_t native_mods);

    void showWindow() override;

    IXFrame* contentWidget() const override;

    void setTheme() override;
protected:
    void dragEnterEvent(QDragEnterEvent *event) override;

    void dragMoveEvent(QDragMoveEvent *event) override;

    void dragLeaveEvent(QDragLeaveEvent *event) override;

    void dropEvent(QDropEvent *event) override;

    void saveAppGeometry() override;

public slots:
    void onThemeChangedFinished(ThemeColor theme_color);

private:    
    bool nativeEvent(const QByteArray& event_type, void* message, qintptr* result) override;

    void showEvent(QShowEvent* event) override;

    void ensureInitTaskbar();

    uint32_t screen_number_;
    QPoint last_pos_;
    QScopedPointer<WinTaskbar> task_bar_;
#if defined(Q_OS_WIN)
    QMap<QString, DriveInfo> exist_drives_;
    QFrame* title_frame_{ nullptr };
#endif
    QScopedPointer<QSystemTrayIcon> tray_icon_;
    QMap<QPair<quint32, quint32>, QKeySequence>  shortcuts_;
    IXFrame *content_widget_;
};
