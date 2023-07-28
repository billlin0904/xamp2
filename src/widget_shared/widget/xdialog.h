//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QDialog>
#include <QToolButton>
#include <FramelessHelper/Widgets/framelessdialog.h>

#include <thememanager.h>

class QGraphicsDropShadowEffect;

FRAMELESSHELPER_USE_NAMESPACE
using namespace wangwenx190::FramelessHelper::Global;

class XAMP_WIDGET_SHARED_EXPORT XDialog : public FramelessDialog {
    Q_OBJECT
public:
    explicit XDialog(QWidget* parent = nullptr);

    void SetContentWidget(QWidget* content, bool transparent_frame = false, bool disable_resize = true);

    QWidget* ContentWidget() const {
        return content_;
    }

    void SetTitle(const QString& title) const;

    void SetIcon(const QIcon& icon) const;
private slots:
    void OnCurrentThemeChanged(ThemeColor theme_color);

private:
#if defined(Q_OS_WIN)
    void showEvent(QShowEvent* event) override;
#endif

    void SetContent(QWidget* content);

    void WaitForReady();

    QLabel* title_frame_label_{ nullptr };
    QWidget* content_{ nullptr };
    QToolButton* icon_{ nullptr };
    QToolButton* close_button_{ nullptr };
    QToolButton* max_win_button_{ nullptr };
    QToolButton* min_win_button_{ nullptr };
    QGraphicsDropShadowEffect* shadow_{ nullptr };
};

