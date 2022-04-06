//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include "xampplayer.h"

#if defined(Q_OS_WIN)
class QScreen;
#endif

class XWindow final : public IXWindow {
public:
    XWindow();

	virtual ~XWindow() override;

    void setContentWidget(IXampPlayer *content_widget);

    void setTaskbarProgress(int32_t percent) override;

    void resetTaskbarProgress() override;

    void setTaskbarPlayingResume() override;

    void setTaskbarPlayerPaused() override;

    void setTaskbarPlayerPlaying() override;

    void setTaskbarPlayerStop() override;
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
private:
    bool nativeEvent(const QByteArray& event_type, void* message, long* result) override;

#if defined(Q_OS_WIN)
    void showEvent(QShowEvent* event) override;
    struct WinTaskbar;
	QPoint last_pos_;
    QScreen* current_screen_;
    QScopedPointer<WinTaskbar> taskbar_;
#endif
    IXampPlayer *content_widget_;
};
