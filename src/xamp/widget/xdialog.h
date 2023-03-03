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

    void SetContentWidget(QWidget* content, bool transparent_frame = false);

    QWidget* ContentWidget() const {
        return frame_->ContentWidget();
    }

    void SetTitle(const QString& title) const {
        frame_->SetTitle(title);
    }

    void SetIcon(const QIcon& icon) const;

    void showEvent(QShowEvent* event) override;
private:
#if defined(Q_OS_WIN)
    void mousePressEvent(QMouseEvent* event) override;

    void mouseReleaseEvent(QMouseEvent* event) override;

    void mouseMoveEvent(QMouseEvent* event) override;

    const int32_t border_width_{ 5 };
    QPoint last_pos_{0, 0};
    QScreen* current_screen_{ nullptr };
#endif
    XFrame* frame_{ nullptr };
    QGraphicsDropShadowEffect* shadow_{ nullptr };
};

