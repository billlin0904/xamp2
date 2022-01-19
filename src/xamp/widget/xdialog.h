//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QDialog>
#include <widget/xframe.h>

class XDialog : public QDialog {
    Q_OBJECT
public:
    explicit XDialog(QWidget* parent = nullptr);

    void setContentWidget(QWidget* content);

    QWidget* contentWidget() const {
        return frame_->contentWidget();
    }

    void setTitle(const QString& title) const {
        frame_->setTitle(title);
    }

private:
    bool nativeEvent(const QByteArray& event_type, void* message, long* result) override;

#if defined(Q_OS_WIN)
    void mousePressEvent(QMouseEvent* event) override;

    void mouseReleaseEvent(QMouseEvent* event) override;

    void mouseMoveEvent(QMouseEvent* event) override;

    bool hitTest(MSG const* msg, long* result) const;

    const int32_t border_width_{ 5 };
    QPoint last_pos_{0, 0};
    QScreen* current_screen_{ nullptr };
#endif
    XFrame* frame_{ nullptr };
};

