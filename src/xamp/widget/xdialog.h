//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QDialog>
#include <widget/xframe.h>

class QGraphicsDropShadowEffect;

class XDialog : public QDialog {
    Q_OBJECT
public:
    explicit XDialog(QWidget* parent = nullptr);

    void SetContentWidget(QWidget* content, bool transparent_frame = false, bool disable_resize = true);

    QWidget* ContentWidget() const {
        return frame_->ContentWidget();
    }

    void SetTitle(const QString& title) const {
        frame_->SetTitle(title);
    }

    void SetMoveable(bool enable) {
        is_moveable_ = enable;
    }

    void SetIcon(const QIcon& icon) const;

private:
#if defined(Q_OS_WIN)
    void showEvent(QShowEvent* event) override;

    void mousePressEvent(QMouseEvent* event) override;

    void mouseReleaseEvent(QMouseEvent* event) override;

    void mouseMoveEvent(QMouseEvent* event) override;

    const int32_t border_width_{ 5 };
    bool is_moveable_{true};
    QPoint last_pos_{0, 0};
    QScreen* current_screen_{ nullptr };
#endif
    XFrame* frame_{ nullptr };
    QGraphicsDropShadowEffect* shadow_{ nullptr };
};

