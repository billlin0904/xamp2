//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
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
    static constexpr auto kMaxTitleHeight = 30;
    static constexpr auto kMaxTitleIcon = 20;
    static constexpr auto kTitleFontSize = 10;

    explicit XDialog(QWidget* parent = nullptr, bool modal = true);

    void setContentWidget(QWidget* content, bool no_moveable = false, bool disable_resize = true);

    QWidget* contentWidget() const {
        return content_;
    }

    void setTitle(const QString& title) const;

    void setIcon(const QIcon& icon) const;

private slots:
    void onThemeChangedFinished(ThemeColor theme_color);

private:
#if defined(Q_OS_WIN)
    void showEvent(QShowEvent* event) override;
#endif

    void closeEvent(QCloseEvent* event) override;

    void setContent(QWidget* content);

    QLabel* title_frame_label_{ nullptr };
    QWidget* content_{ nullptr };
    QToolButton* icon_{ nullptr };
    QToolButton* close_button_{ nullptr };
    QToolButton* max_win_button_{ nullptr };
    QToolButton* min_win_button_{ nullptr };
    QFrame* title_frame_{ nullptr };
    QGraphicsDropShadowEffect* shadow_{ nullptr };
};

