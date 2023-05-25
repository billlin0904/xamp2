//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QDialog>
#include <FramelessHelper/Widgets/framelessdialog.h>
#include <widget/xframe.h>

class XAMP_WIDGET_SHARED_EXPORT QGraphicsDropShadowEffect;

FRAMELESSHELPER_USE_NAMESPACE

class XAMP_WIDGET_SHARED_EXPORT XDialog : public FramelessDialog {
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

    void SetIcon(const QIcon& icon) const;

private:
#if defined(Q_OS_WIN)
    void showEvent(QShowEvent* event) override;
#endif
    XFrame* frame_{ nullptr };
    QGraphicsDropShadowEffect* shadow_{ nullptr };
};

