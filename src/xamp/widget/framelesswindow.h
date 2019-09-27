//=====================================================================================================================
// Copyright (c) 2018-2019 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QWidget>

#include <QMimeData>
#include <QDragEnterEvent>
#include <QSystemTrayIcon>

#if defined(Q_OS_WIN)
class QWinThumbnailToolBar;
class QWinTaskbarButton;
class QWinTaskbarProgress;
#endif

class FramelessWindow : public QWidget {
public:
    explicit FramelessWindow(QWidget *parent = nullptr);

    virtual ~FramelessWindow();

    Q_DISABLE_COPY(FramelessWindow)

    virtual void addDropFileItem(const QUrl& url);

    virtual void removeSelectedItem();

	void setTaskbarProgress(double percent) const;

	void resetTaskbarProgress() const;

	void setTaskbarPlayingResume() const;

	void setTaskbarPlayerPaused() const;

	void setTaskbarPlayerPlaying() const;

	void setTaskbarPlayerStop() const;

protected:
    bool eventFilter(QObject* object, QEvent* event);

    void dragEnterEvent(QDragEnterEvent *event) override;

    void dragMoveEvent(QDragMoveEvent *event) override;

    void dragLeaveEvent(QDragLeaveEvent *event) override;

    void dropEvent(QDropEvent *event) override;

    bool hitTest(MSG const* msg, long* result) const;

    void mousePressEvent(QMouseEvent *event) override;

    void mouseReleaseEvent(QMouseEvent * event) override;

    void mouseMoveEvent(QMouseEvent * event) override;

	void showEvent(QShowEvent * event) override;

	virtual void play();

	virtual void playNextClicked();

	virtual void playPreviousClicked();

	virtual void stopPlayedClicked();

private:
	void initialFontDatabase();

#if defined(Q_OS_WIN)
	bool nativeEvent(const QByteArray& event_type, void* message, long* result) override;

	bool is_maximized_;
	int32_t border_width_;
	std::unique_ptr<QWinThumbnailToolBar> thumbnail_tool_bar_;
	std::unique_ptr<QWinTaskbarButton> taskbar_button_;
	QWinTaskbarProgress* taskbar_progress_;
#endif
	QIcon play_icon_;
	QIcon pause_icon_;
	QIcon stop_play_icon_;
	QIcon seek_forward_icon_;
	QIcon seek_backward_icon_;
	QMargins frames_;
	QMargins margins_;
    QSystemTrayIcon tray_icon_;
};
