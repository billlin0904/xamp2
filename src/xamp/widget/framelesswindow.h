//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QWidget>
#include <QMimeData>
#include <QIcon>
#include <QDragEnterEvent>

#include "xampplayer.h"

#if defined(Q_OS_WIN)
class QWinThumbnailToolBar;
class QWinTaskbarButton;
class QWinTaskbarProgress;
class QScreen;
#endif

class FramelessWindow final : public ITopWindow {
public:
    explicit FramelessWindow(bool use_native_window);

	virtual ~FramelessWindow() override;

    void initial(IXampPlayer *content_widget);

    void setTaskbarProgress(int32_t percent) override;

    void resetTaskbarProgress() override;

    void setTaskbarPlayingResume() override;

    void setTaskbarPlayerPaused() override;

    void setTaskbarPlayerPlaying() override;

    void setTaskbarPlayerStop() override;

    [[nodiscard]] bool useNativeWindow() const noexcept override {
		return use_native_window_;
	}
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

    void changeEvent(QEvent * event) override;

	void paintEvent(QPaintEvent * event) override;

    void closeEvent(QCloseEvent* event) override;
private:
	void createThumbnailToolBar();

    bool nativeEvent(const QByteArray& event_type, void* message, long* result) override;	

	bool use_native_window_;

#if defined(Q_OS_WIN)
	int32_t border_width_;
	QIcon play_icon_;
	QIcon pause_icon_;
	QIcon stop_play_icon_;
	QIcon seek_forward_icon_;
	QIcon seek_backward_icon_;
	QPoint last_pos_;
    QScreen* current_screen_;
	QScopedPointer<QWinThumbnailToolBar> thumbnail_tool_bar_;
	QScopedPointer<QWinTaskbarButton> taskbar_button_;
	QWinTaskbarProgress* taskbar_progress_;
#endif
    IXampPlayer *content_widget_;
};
