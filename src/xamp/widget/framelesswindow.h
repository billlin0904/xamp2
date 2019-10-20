//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QWidget>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QSystemTrayIcon>
#include <QMainWindow>

#include <base/align_ptr.h>

#if defined(Q_OS_WIN)
class QWinThumbnailToolBar;
class QWinTaskbarButton;
class QWinTaskbarProgress;
#endif

class FramelessWindow : public QWidget {
public:
    explicit FramelessWindow(QWidget *parent = nullptr);

    ~FramelessWindow() override;

    Q_DISABLE_COPY(FramelessWindow)

    virtual void addDropFileItem(const QUrl& url);

    virtual void onDeleteKeyPress();

	void setTaskbarProgress(double percent);

	void resetTaskbarProgress();

	void setTaskbarPlayingResume();

	void setTaskbarPlayerPaused();

	void setTaskbarPlayerPlaying();

	void setTaskbarPlayerStop();

protected:
    bool eventFilter(QObject* object, QEvent* event) override;

    void dragEnterEvent(QDragEnterEvent *event) override;

    void dragMoveEvent(QDragMoveEvent *event) override;

    void dragLeaveEvent(QDragLeaveEvent *event) override;

    void dropEvent(QDropEvent *event) override;
#if defined(Q_OS_WIN)
    bool hitTest(MSG const* msg, long* result) const;
#endif
    void mousePressEvent(QMouseEvent *event) override;

    void mouseReleaseEvent(QMouseEvent * event) override;

    void mouseMoveEvent(QMouseEvent * event) override;

	void showEvent(QShowEvent * event) override;

    virtual void play() = 0;

    virtual void playNextClicked() = 0;

    virtual void playPreviousClicked() = 0;

    virtual void stopPlayedClicked() = 0;

private:
	void lazyInitial();

	void initialFontDatabase();

    bool nativeEvent(const QByteArray& event_type, void* message, long* result) override;

#if defined(Q_OS_WIN)
	bool is_maximized_;
	int32_t border_width_;
	QIcon play_icon_;
	QIcon pause_icon_;
	QIcon stop_play_icon_;
	QIcon seek_forward_icon_;
	QIcon seek_backward_icon_;
	xamp::base::AlignPtr<QWinThumbnailToolBar> thumbnail_tool_bar_;
	xamp::base::AlignPtr<QWinTaskbarButton> taskbar_button_;
	QWinTaskbarProgress* taskbar_progress_;
#endif
};
